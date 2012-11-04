import os
import re

result_dir = "./Results/"
missing_data_file = os.path.join(os.getcwd() ,"MissingResults")
speedups_file = os.path.join(os.getcwd(), "Speedups")

files = os.listdir(result_dir)

invalid_data = {}

speedups_file = open(speedups_file, 'w')

timers = ["Weight generation", 
          "Normalisation", 
          "Table construction", 
          "Sampling"
          ]
                    
for curfile in files:
    
    path = os.path.join(result_dir, curfile)
    
    if os.path.isdir(path):

        speedups_file.write(str(path) + "\n")

        data = {}

        procss = set()

        filenames = os.listdir(path)
        
        for filename in filenames:

            mode, procs, pipe = filename.split(".")
            
            procs = int(procs)
                        
            if pipe == "o":
                
                f = open(os.path.join(path, filename))
                
                filedata = f.read() 

                for line in filedata.split("\n"):

                    for timer in timers:
                        time = re.split(r'' + timer + ': ', line)

                        if len(time) > 1:
                            
                            time = float(time[1][:-1])
                        
                            procss.add(procs)
                            
                            if not data.has_key(mode):
                                data[mode] = {}

                            if not data[mode].has_key(timer):
                                data[mode][timer] = {}

                            data[mode][timer][procs] = time

                        
        procss = sorted(procss)

        speedups = {}
        
        for mode in data:

            if mode == "Sequential":
                continue
            
            line = mode + ","
            for procs in procss:
                line += "%d," % procs
                
            speedups_file.write(line + "\n")

            for timer in data[mode]:

                seqdata = data["Sequential"][timer]
                modedata = data[mode][timer]
            
                line = timer + ","
                SeqTime = None
    
                if not seqdata.has_key(1):
                    invalid_data[(curfile, "Sequential", 1)] = "Missing"
                else:
                    SeqTime = seqdata[1]
                
                if SeqTime == 0:
                    invalid_data[(curfile, "Sequential", 1)] = "Null Data"
                    
                for procs in procss:
                    
                    SpecTime = None

                    if not modedata.has_key(procs):
                        invalid_data[(curfile, mode, procs)] = "Missing"
                        
                    else:
                        SpecTime = modedata[procs]
                            
                    if not SeqTime or not SpecTime:
                        line += ","
                    
                    elif SpecTime == 0:
                        invalid_data[(curfile, mode, procs)] = "Null Data"
                        line += ","
                    
                    else:
                        speedup = SeqTime / SpecTime
                        line += "%f," % speedup
                    
                speedups_file.write(line + "\n")

speedups_file.close()

if os.path.exists(missing_data_file):
    os.remove(missing_data_file)
    
f = open(missing_data_file, 'w')

for entry in sorted(invalid_data):
    folder, mode, procs = entry;
    host = folder.split("-")[0]
    line = host + "," + mode + "," + str(procs) + "\n"
    f.write(line) 
    
f.close() 
