import sys, os, struct

stdOut = open(sys.stdout.fileno(),'wb')
stdErr = open(sys.stderr.fileno(), 'w')
stdErr.write("python begins")

with open('testProgram.vert.spv') as vert:
    #get fileSize and return
    stdErr.write('\npython return filezise\n')
    stdOut.write(struct.pack('i',os.path.getsize('testProgram.vert.spv')))
    stdOut.write(struct.pack('f',os.path.getsize('testProgram.frag.spv')))
    #stdOut.write(struct.pack('i',os.path.getsize('testProgram.vert.spv')))

    stdErr.write('\npython awaits\n')
    #input()
    

sys.stdin.close()
sys.stdout.close()

sys.exit(0)