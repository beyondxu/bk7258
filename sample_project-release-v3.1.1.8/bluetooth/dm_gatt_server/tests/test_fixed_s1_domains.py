"""Run the actual domain controller with S1 selected and independent byte oracle."""
from pathlib import Path
import os, subprocess, sys
subprocess.run([sys.executable, str(Path(__file__).with_name('test_fixed_s2_domains.py'))],
               env={**os.environ, 'PRINTER_TEST_S1': '1'}, check=True)
