# memez
A safe reinterpretation of https://github.com/Leurak/MEMZ
Memez does not corrupt your mbr, delete files, or cause any harmful side-effects.
Works on windows.

## usage
./memez

## installation
You can build with msvs as usual. Compilation by hand is also possible; `c++ -o memez memez.cpp`.

# as a service
1) edit the install.ps1 and add the target username to the $username variable
2) run install.ps1 from the root folder of the project.
To verify that it was installed run `Get-ScheduledTask` and look for the memez*. `Get-ScheduledTask` also takes a -Name flag.

## TODO:
* DONE ~Prevent users from killing memez.~
Just install script as admin
* DONE ~Create a windows service that runs memez at a certain time/day.~
The install.ps1 does that
* DONE ~BSOD after memez has finished.~
Oh boy does it.
