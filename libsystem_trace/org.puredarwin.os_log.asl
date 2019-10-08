? [T os_log(3)] claim
> /var/log/libtrace_messages.asl soft compress rotate=seq ttl=30 format=asl
> /var/log/libtrace_messages.log soft compress rotate=seq ttl=30 format='$((Time)(JZ)) [$(Sender) ($(PID))]: $(Subsystem)/$(Category) $((Level)(str)): $(Message)'
? [T os_log(3)] file /var/log/libtrace_messages.asl
? [T os_log(3)] file /var/log/libtrace_messages.log
