all: master obstacles targets watchdog
clean-logs:
	rm log/logfile.txt
	touch log/logfile.txt
clean:
	rm build/*
master:
	gcc master.c -o build/master 
	
obstacles:
	gcc obstacles.c -o build/obstacles 

targets:
	gcc targets.c -o build/targets -lm
	
watchdog:
	gcc watchdog.c -o build/watchdog -lncurses


