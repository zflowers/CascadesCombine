source /cvmfs/cms.cern.ch/cmsset_default.sh
export CMSSW_GIT_REFERENCE=/cvmfs/cms.cern.ch/cmssw.git.daily
source /cvmfs/cms.cern.ch/crab3/crab.sh
source /cvmfs/cms.cern.ch/rucio/setup-py3.sh
alias proxy='voms-proxy-init --voms cms -valid 192:00'
alias sl7='cd /uscms/home/z374f439/nobackup/ && cmssw-el7 --bind /uscms_data/d1/z374f439/'
alias cascades='cd /uscms/home/z374f439/nobackup/CMSSW_13_3_1/src/SUSYCascades/; cmsenv; source scripts/setup_RestFrames_connect.sh'
alias setup_combine='export SCRAM_ARCH=el9_amd64_gcc12 && cd /uscms/home/z374f439/nobackup/CMSSW_14_1_0_pre4/src/CascadesCombine/ && cmsenv'
alias condor_rm_user='condor_rm $USER -n lpcschedd4; condor_rm $USER -n lpcschedd5; condor_rm $USER -n lpcschedd6'
alias condor='watch condor_q $USER -batch'
run_combine() {
    nohup python3 python/run_combine.py "$@" > debug_run_combine.debug 2>&1 & 
}
