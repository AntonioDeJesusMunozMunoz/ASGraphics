import sys, os, struct

stdOut = open(sys.stdout.fileno(),'wb')
stdErr = open(sys.stderr.fileno(), 'w')
stdErr.write("python begins")

#get file to read
stdErr.write('\npython reading file name\n')
fileName = input()

with open(fileName, 'rb') as vert:
    #get fileSize and return
    stdErr.write('\npython return filezise\n')
    stdOut.write(struct.pack('i',os.path.getsize(fileName)))    

    stdErr.write('\npython awaits\n')# false
    #_a = input()
    
    #return file contents
    stdErr.write('\npython return vertexContents\n')
    stdOut.write(os.read(vert.fileno(),os.path.getsize(fileName)))    

stdErr.write('\npython awaits permision to end\n')
_a = input()
sys.stdin.close()
sys.stdout.close()

sys.exit(0)