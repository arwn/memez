## This script deploy memez as a service
$username = "42" # change username to the target user
# don't change anything below here

$filepath = 'C:\memez.exe'

# chown file
icacls $filepath /setowner admin

# create  task
$trigger = New-ScheduledTaskTrigger -At 11pm -Weekly -DaysOfWeek monday, tuesday, wednesday, thursday, saturday, sunday
$triggerFriday = New-ScheduledTaskTrigger -At 12pm -Weekly -DaysOfWeek friday
$action = New-ScheduledTaskAction -execute $filepath
Register-ScheduledTask -Action $action -Trigger $trigger -User $username -TaskName "memez" -Description "Anti gamer script"
Register-ScheduledTask -Action $action -Trigger $triggerFriday -User $username -TaskName "memezf" -Description "Anti gamer script, on friday"
