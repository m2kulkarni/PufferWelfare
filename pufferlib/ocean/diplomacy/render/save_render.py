"""Save a render of the current game state to a PNG file."""
import os
import sys

# For headless rendering, we need to set up a virtual display
# Try to use xvfb if available
try:
    import subprocess
    # Check if we're in a headless environment
    if not os.environ.get('DISPLAY'):
        # Try to start Xvfb
        os.environ['DISPLAY'] = ':99'
        subprocess.Popen(['Xvfb', ':99', '-screen', '0', '1920x1080x24'],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        import time
        time.sleep(0.5)  # Give Xvfb time to start
except Exception as e:
    print(f"Note: Could not set up virtual display: {e}")

from PIL import Image
import numpy as np

def save_render(output_path="render_output.png"):
    """Render the game state and save to a PNG file."""
    from pufferlib.ocean.diplomacy import Diplomacy, binding
    from pufferlib.ocean.diplomacy.render.renderer import DiplomacyRenderer, RAYLIB_AVAILABLE

    if not RAYLIB_AVAILABLE:
        print("Raylib not available")
        return

    # Create environment
    env = Diplomacy()
    env.reset()

    # Create renderer
    renderer = DiplomacyRenderer(env.env_handle, screen_width=1200, screen_height=900)

    # Render one frame and get the pixels
    pixels = renderer.render()

    # Close renderer
    renderer.close()

    if pixels is not None:
        # Convert to PIL Image and save
        # Raylib returns RGBA, PIL expects RGB or RGBA
        img = Image.fromarray(pixels[:, :, :3])  # Take RGB channels
        img.save(output_path)
        print(f"Saved render to: {output_path}")
        return output_path
    else:
        print("Failed to capture render")
        return None


if __name__ == "__main__":
    output = sys.argv[1] if len(sys.argv) > 1 else "/scratch/mmk9418/projects/PufferWelfare/render_output.png"
    save_render(output)
