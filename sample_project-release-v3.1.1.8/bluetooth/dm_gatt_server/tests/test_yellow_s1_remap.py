"""Run exact APP yellow controller with the independent S1 relocation oracle."""
from pathlib import Path
import os,subprocess,sys
subprocess.run([sys.executable,str(Path(__file__).with_name('test_app_yellow_only.py')),*sys.argv[1:]],
               env={**os.environ,'PRINTER_TEST_S1_REMAP':'1'},check=True)
