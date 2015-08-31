import os,sys
import ConfigParser
import subprocess

def stabilize(config):
    '''
        Use Cuda -gpu=(yes|no)
        get rid of wobbling -ws=(yes|no)
        deblur images --deblur=(yes|no)
        --output=(no|filepath)
    '''
    cmd = "./cpp-example-videostab %s -gpu=%s -ws=%s --deblur=%s --quiet --output=%s"%()
    process = subprocess.Popen(cmd, shell=True)#,stdout=subprocess.PIPE
    #for line in process.stdout: print line
    process.wait()
    return process.returncode

def stabilizeReg(config):
    '''
        Use Cuda -gpu=(yes|no)
        get rid of wobbling -ws=(yes|no)
        deblur images --deblur=(yes|no)
        --output=(no|filepath)
    '''
    inputFile,outputFile,gpu,ws,deblur=config
    #print inputFile,outputFile,gpu,ws,deblur
    cmd = "./cpp-example-videostab %s -gpu=%s -ws=%s --deblur=%s --quiet --output=%s"%(inputFile,gpu,ws,deblur,outputFile)
    process = subprocess.Popen(cmd, shell=True)#,stdout=subprocess.PIPE
    #for line in process.stdout: print line
    process.wait()
    return process.returncode

if __name__=='__main__':
    print "Stabilizing %s"%sys.argv[1]
    inputFile = sys.argv[1]
    outputFile = sys.argv[2]
    gpu = sys.argv[3]
    ws = sys.argv[4]
    deblur = sys.argv[5]
    print stabilizeReg((inputFile,outputFile,gpu,ws,deblur))
