import datetime
import os
import socket
import subprocess
import time
import threading
import multiprocessing

working_directory = os.path.join(os.getcwd() ,"./")

results_directory = os.path.join(os.getcwd() ,"./Results/")

procs = range(multiprocessing.cpu_count(),0,-1)

max_attempts = 1
min_runtime = 2

"""
    kill_process
"""
def kill_process(process):
    global retry
    retry = True

    try:
        process.terminate()
    except OSError:
        pass
        
"""
    run
"""
def run(command,working_directory,max_time = None):

    print "Running \"" + working_directory + str(command) + "\""
    global retry
    retry = True
    attempt = 0
    
    while retry and attempt < max_attempts:

        retry = False
        attempt += 1

        try:
            start_time = time.time()
            
            process = subprocess.Popen(command,
                                       shell=False,
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE,
                                       cwd=working_directory)

            if max_time:
                timer = threading.Timer(max_time,kill_process,args=[process])
                timer.start()

            output = process.communicate()
            
            end_time = time.time()

        finally:
            if max_time and timer:
                timer.cancel()            
            
    if retry:
        return None
    else:
        return (process.returncode, attempt, output, end_time - start_time)

def runSeqBenches(working_directory, name):

    output = run(["./sequential"], working_directory)
    
    stdout = open(path + name + ".1.o", 'w')
    stdout.write(output[2][0])
    stdout.close()
    
    stderr = open(path + name + ".1.e", 'w')
    stderr.write(output[2][1])
    stderr.close()

def runBenches(procs, working_directory, name):

    output = run(["./parallel", str(procs)], working_directory)
    
    stdout = open(path + name + "." + str(proc) + ".o", 'w')
    stdout.write(output[2][0])
    stdout.close()
    
    stderr = open(path + name + "." + str(proc) + ".e", 'w')
    stderr.write(output[2][1])
    stderr.close()

"""
    Start of program
"""
if not os.path.exists(results_directory):
    os.makedirs(results_directory)

now = str(datetime.datetime.now().strftime("%Y-%m-%d.%H-%M"))
hostname = socket.gethostname()

path = results_directory + hostname + "-" + str(now) + "/"
extra = 1

while(os.path.exists(path)):
    path = results_directory + hostname + "-" + str(now) + "(" + str(extra) + ")/"
    extra += 1

os.makedirs(path)

run(["make"], working_directory)

# Sequential Execution
runSeqBenches(working_directory, "Sequential")

# Parallel Execution
for proc in procs:
    runBenches(proc, working_directory, "Parallel")


