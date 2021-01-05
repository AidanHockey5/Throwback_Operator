NO DEPENDANCIES OR PYTHON INSTALL REQUIRED FOR WINDOWS EXE



PYTHON DEPENDANCIES:
Python 3.0+
pyserial
requests

Enter the following commands:

pip install pyserial
pip install requests


Run:
python3 Flash.py

----------------------------------------------

Instructions for use:

On the THROWBACK OPERATOR DEVICE: Move the left jumper pin at the top of the board to the "UP" position.
It should look like this:
https://www.aidanlawrence.com/wp-content/uploads/2021/01/tbojumper-scaled.jpg

Plug-in your THROWBACK OPERATOR to your computer via USB.

Press the RESET BUTTON.

Run the Flash.exe program OR run Flash.py with Python.

Select the COM port of the THROWBACK OPERATOR.

If everything is correct, your system should automatically download the firmware file and flash.

After flashing, replace the jumper's position back to DOWN.

----------------------------------------------

EXE Built with pyinstaller
pyinstaller --onefile Flash.py