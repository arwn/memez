## This script deploy memez as a service
# set path to exe
$filepath = 'C:\memez.exe'

# chown file
icacls $filepath /setowner admin

# create  task
$trigger = New-ScheduledTaskTrigger -AtLogOn
$action = New-ScheduledTaskAction -execute $filepath
Register-ScheduledTask -Action $action -Trigger $trigger -TaskName "memez" -Description "Anti gamer script"