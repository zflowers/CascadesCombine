#!/bin/bash
# Wrapper for HTCondor job at LPC

echo "[$(date)] Starting job on $(hostname)"
echo "[$(date)] Working directory: $(pwd)"

# Source CMS environment
source /cvmfs/cms.cern.ch/cmsset_default.sh

# Set architecture
export SCRAM_ARCH=el9_amd64_gcc12

# Load CMSSW release from CVMFS
cmsenv_dir=/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_14_1_0_pre4/src
if [ -d "$cmsenv_dir" ]; then
    cd $cmsenv_dir
    eval `scram runtime -sh`
    cd -
else
    echo "CMSSW release not found on CVMFS!"
    exit 1
fi

# Use the forwarded X509 proxy
echo "Using proxy: $X509_USER_PROXY"
export X509_USER_PROXY=$X509_USER_PROXY

# Make sure executable is runnable
chmod +x BFI.x
mkdir -p json/

# Run the executable
echo "Running BFI.x"
stdbuf -oL -eL ./BFI.x 2>&1 | tee BFI.debug

echo "[$(date)] Job finished."

