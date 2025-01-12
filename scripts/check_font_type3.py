import subprocess
from pathlib import Path


# Check if all figures in the figures directory do not contain type 3 fonts
# This is because EDAS does not accept type 3 fonts in submitted PDFs
all_good = True
for figure in Path("figures").iterdir():
    if not figure.name.endswith(".pdf"):
        continue
    result = subprocess.run(["pdffonts", figure], capture_output=True, text=True)
    if "Type 3" in result.stdout:
        print(f"{figure.name} contains type 3 fonts")
        all_good = False
if all_good:
    print("All figures do not contain type 3 fonts")
