"""Verify one CMY prefix precedes the unchanged actual APP yellow frames."""
from pathlib import Path
import os,subprocess,sys
subprocess.run([sys.executable,str(Path(__file__).with_name('test_app_yellow_only.py')),*sys.argv[1:]],
 env={**os.environ,'PRINTER_TEST_S1_REMAP':'1','PRINTER_TEST_BLACK_PREFIX':'1'},check=True)
