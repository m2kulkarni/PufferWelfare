// bindings_cpu.cpp - CPU-only Python bindings (no nvcc/CUDA required)

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#define _PUFFER_STRINGIFY(x) #x
#define PUFFER_STRINGIFY(x) _PUFFER_STRINGIFY(x)
#include <cstring>
#include <memory>
#include <atomic>
#include <cmath>

// vecenv.h header section gives us StaticVec, Dict, cudaStream_t typedef
#include "vecenv.h"

namespace py = pybind11;

// Stub out CUDA functions that the static lib references (dead code when gpu=0)
extern "C" {
typedef int cudaError_t;
typedef int cudaMemcpyKind;
cudaError_t cudaHostAlloc(void**, size_t, unsigned int) { return 0; }
cudaError_t cudaMalloc(void**, size_t) { return 0; }
cudaError_t cudaMemcpy(void*, const void*, size_t, cudaMemcpyKind) { return 0; }
cudaError_t cudaMemcpyAsync(void*, const void*, size_t, cudaMemcpyKind, cudaStream_t) { return 0; }
cudaError_t cudaMemset(void*, int, size_t) { return 0; }
cudaError_t cudaFree(void*) { return 0; }
cudaError_t cudaFreeHost(void*) { return 0; }
cudaError_t cudaSetDevice(int) { return 0; }
cudaError_t cudaDeviceSynchronize(void) { return 0; }
cudaError_t cudaStreamSynchronize(cudaStream_t) { return 0; }
cudaError_t cudaStreamCreateWithFlags(cudaStream_t*, unsigned int) { return 0; }
cudaError_t cudaStreamQuery(cudaStream_t) { return 0; }
const char* cudaGetErrorString(cudaError_t) { return "stub"; }
}

// ============================================================================
// CPU advantage (same as puff_advantage_row_scalar but plain C++)
// ============================================================================

static void py_puff_advantage_cpu(
        long long values_ptr, long long rewards_ptr,
        long long dones_ptr, long long importance_ptr,
        long long advantages_ptr,
        int num_steps, int horizon,
        float gamma, float lambda, float rho_clip, float c_clip) {
    const float* values = (const float*)values_ptr;
    const float* rewards = (const float*)rewards_ptr;
    const float* dones = (const float*)dones_ptr;
    const float* importance = (const float*)importance_ptr;
    float* advantages = (float*)advantages_ptr;
    for (int row = 0; row < num_steps; row++) {
        int off = row * horizon;
        float lastpufferlam = 0;
        for (int t = horizon - 2; t >= 0; t--) {
            int t_next = t + 1;
            float nextnonterminal = 1.0f - dones[off + t_next];
            float imp = importance[off + t];
            float rho_t = imp < rho_clip ? imp : rho_clip;
            float c_t = imp < c_clip ? imp : c_clip;
            float r_nxt = rewards[off + t_next];
            float v = values[off + t];
            float v_nxt = values[off + t_next];
            float delta = rho_t * r_nxt + gamma * v_nxt * nextnonterminal - v;
            lastpufferlam = delta + gamma * lambda * c_t * lastpufferlam * nextnonterminal;
            advantages[off + t] = lastpufferlam;
        }
    }
}

// ============================================================================
// Dict helpers (same as bindings.cu)
// ============================================================================

static int integer_config(py::dict kwargs, const char* key, int low, int high) {
    if (!kwargs.contains(key))
        throw py::value_error(std::string("Missing config key: ") + key);
    py::handle value = kwargs[key];
    if (!PyLong_Check(value.ptr()) || PyBool_Check(value.ptr()))
        throw py::value_error(std::string(key) + " must be an integer");
    int overflow = 0;
    long long number = PyLong_AsLongLongAndOverflow(value.ptr(), &overflow);
    if (overflow || number < low || number > high)
        throw py::value_error(std::string(key) + " is outside the supported range");
    return static_cast<int>(number);
}

static std::atomic<int> live_dicts{0};
static std::atomic<int> live_vectors{0};
struct DictDeleter {
    void operator()(Dict* dict) const noexcept {
        if (!dict) return;
        // Keys are borrowed from the Python dictionary, never owned here.
        free(dict->items);
        free(dict);
        --live_dicts;
    }
};
using OwnedDict = std::unique_ptr<Dict, DictDeleter>;
static OwnedDict owned_dict(int capacity) {
    OwnedDict result(create_dict(capacity));
    ++live_dicts;
    return result;
}
static OwnedDict py_dict_to_c_dict(py::dict py_dict) {
    auto c_dict = owned_dict(py_dict.size());
    for (auto item : py_dict) {
        const char* key = PyUnicode_AsUTF8(item.first.ptr());
        if (!key) throw py::error_already_set();
        try { dict_set(c_dict.get(), key, item.second.cast<double>()); }
        catch (const py::cast_error&) {}
    }
    return c_dict;
}

// ============================================================================
// VecEnv wrapper
// ============================================================================

struct VecEnv {
    StaticVec* vec = nullptr;
    ~VecEnv() { close(); }
    void close() noexcept {
        if (!vec) return;
        static_vec_close(vec);
        vec = nullptr;
        --live_vectors;
    }
    void require_open() const {
        if (!vec) throw std::runtime_error("VecEnv is closed");
    }
    int total_agents;
    int obs_size;
    int num_atns;
    std::vector<int> act_sizes;
    std::string obs_dtype;
    size_t obs_elem_size;
};

static std::unique_ptr<VecEnv> create_vec(py::dict args, int gpu = 0) {
    if (gpu != 0) throw py::value_error("This backend supports CPU only");
    py::dict vec_kwargs = args["vec"].cast<py::dict>();
    py::dict env_kwargs = args["env"].cast<py::dict>();
    // Validate before allocating any engine or binding resources.
    int total_agents = integer_config(vec_kwargs, "total_agents", 7, 28672);
    if (total_agents % 7)
        throw py::value_error("total_agents must contain complete seven-power games");
    int num_buffers = integer_config(vec_kwargs, "num_buffers", 1, 1);
    if (env_kwargs.contains("max_years"))
        integer_config(env_kwargs, "max_years", 1, 1000);
    if (env_kwargs.contains("welfare_mode")) {
        py::handle mode = env_kwargs["welfare_mode"];
        if (!PyBool_Check(mode.ptr())) integer_config(env_kwargs, "welfare_mode", 0, 1);
    }
    auto vec_dict = py_dict_to_c_dict(vec_kwargs);
    auto env_dict = py_dict_to_c_dict(env_kwargs);

    auto ve = std::make_unique<VecEnv>();
    {
        py::gil_scoped_release no_gil;
        ve->vec = create_static_vec(total_agents, num_buffers, 0, vec_dict.get(), env_dict.get());
        if (ve->vec) ++live_vectors;
    }
    ve->total_agents = total_agents;
    ve->obs_size = get_obs_size();
    ve->num_atns = get_num_atns();
    {
        int* raw = get_act_sizes();
        int n = get_num_act_sizes();
        ve->act_sizes = std::vector<int>(raw, raw + n);
    }
    ve->obs_dtype = std::string(get_obs_dtype());
    ve->obs_elem_size = get_obs_elem_size();
    return ve;
}

static void vec_reset(VecEnv& ve) {
    ve.require_open();
    // Keep the GIL: another Python thread must not close during this call.
    static_vec_reset(ve.vec);
}

static void cpu_vec_step_py(VecEnv& ve, long long actions_ptr) {
    ve.require_open();
    memcpy(ve.vec->actions, (void*)actions_ptr,
        (size_t)ve.total_agents * ve.num_atns * sizeof(float));
    // The vector can be closed by another Python thread only after this call.
    cpu_vec_step(ve.vec);
}

static py::dict vec_log(VecEnv& ve) {
    ve.require_open();
    auto out = owned_dict(32);
    static_vec_log(ve.vec, out.get());
    py::dict result;
    for (int i = 0; i < out->size; i++)
        result[out->items[i].key] = out->items[i].value;
    return result;
}

static void vec_close(VecEnv& ve) {
    ve.close();
}

// ============================================================================
// Module
// ============================================================================

PYBIND11_MODULE(_C, m) {
    // Private diagnostics count binding-owned resources, not total engine memory.
    m.def("_resource_counts", []() {
        return py::make_tuple(live_dicts.load(), live_vectors.load());
    });
    m.attr("precision_bytes") = 4;
    m.attr("env_name") = PUFFER_STRINGIFY(ENV_NAME);
    m.attr("gpu") = 0;

    m.def("puff_advantage_cpu", &py_puff_advantage_cpu);
    m.def("create_vec", &create_vec, py::arg("args"), py::arg("gpu") = 0);

    py::class_<VecEnv, std::unique_ptr<VecEnv>>(m, "VecEnv")
        .def_readonly("total_agents", &VecEnv::total_agents)
        .def_readonly("obs_size", &VecEnv::obs_size)
        .def_readonly("num_atns", &VecEnv::num_atns)
        .def_readonly("act_sizes", &VecEnv::act_sizes)
        .def_readonly("obs_dtype", &VecEnv::obs_dtype)
        .def_readonly("obs_elem_size", &VecEnv::obs_elem_size)
        .def_property_readonly("gpu", [](VecEnv&) { return 0; })
        .def_property_readonly("obs_ptr", [](VecEnv& ve) { ve.require_open(); return (long long)ve.vec->observations; })
        .def_property_readonly("rewards_ptr", [](VecEnv& ve) { ve.require_open(); return (long long)ve.vec->rewards; })
        .def_property_readonly("terminals_ptr", [](VecEnv& ve) { ve.require_open(); return (long long)ve.vec->terminals; })
        .def("reset", &vec_reset)
        .def("cpu_step", &cpu_vec_step_py)
        .def("render", [](VecEnv& ve, int env_id) { ve.require_open(); static_vec_render(ve.vec, env_id); })
        .def("log", &vec_log)
        .def("close", &vec_close);
}
