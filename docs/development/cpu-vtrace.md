# CPU importance weighting

Espeholt et al., *IMPALA: Scalable Distributed Deep-RL with Importance Weighted
Actor-Learner Architectures* (ICML 2018, arXiv:1802.01561), V-trace equations (1)
and (2), define the clipped importance weight on the entire temporal-difference
residual: rho * (reward + gamma * next_value - value). The local CPU implementation
now follows that placement. Existing terminal masking and lambda-weighted trace
recursion are unchanged.

Primary paper mirror:
https://www.cl.cam.ac.uk/~ey204/teaching/ACS/R244_2020_2021/papers/Espeholt_ICML_2018.pdf

Tests compare the compiled function with an independent scalar recurrence using
rho values 0, 0.25, 1 and 2, nonzero values, terminal/nonterminal transitions,
clipping and a multistep trace. Unit-ratio regression expectations are unchanged.
The imported CPU build contains no GPU kernels. This addresses the CPU formula
portion of issue #24; upstream CUDA scalar/SIMD execution and parity remain
unverified. GPU training must remain unsupported in this package.
