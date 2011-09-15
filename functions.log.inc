log_date_format="%H:%M:%S";
#date_format="%Y.%m.%d %H:%M:%S";
_date="";

## _date_format
#  Sets the '_date' variable to the format specified in 'date_format';
function _date_format() {
  _date=$( date +"$log_date_format" );
}

log_format="$log_format";
log_format_verbose="$log_format_verbose";

# Use %color and %reset to colorfy the levels
if [[ -z "$log_format" ]]; then
  log_format="%date - [%color%level%reset]: %message";
fi;

if [[ -z "$log_format_verbose" ]]; then
  log_format_verbose="%date - %method - %level: %message";
fi;

_log_formatted="";

modulelog_prefix="";
modulelog_module="";
function log_module() {
    if [[ "$verbose" == "1" ]]; then
        log_info "$modulelog_module" "${modulelog_prefix}$1";
    else
        log_info "$modulelog_module" "${modulelog_prefix}$1" "$PREVIOUS";
    fi;
}

function log_info() {
    if [[ "$quiet" == "1" ]]; then
        return 0;
    else
      _log "INFO" "$BGREEN" "$1" "$2" "$3";
      echo -e "$_log_formatted";
    fi;
}

function log_warn() {
  _log "WARNING" "$BYELLOW" "$1" "$2" "$3";
  echo -e "$_log_formatted" 1>&2;
}

function log_error() {
  _log "ERROR" "$BRED" "$1" "$2" "$3";
  echo -e "$_log_formatted" 1>&2;
}


function log_debug() {
    if [[ "$quiet" == "1" ]]; then
        return 0;
    fi;
    if [[ $verbose == "1" ]]; then
      _log "INFO" "$BGREEN" "$1" "$2" "$3";
      echo -e "$_log_formatted";
  fi;
}

function log_command() {
    local level="$1";
    local name="$2";
    shift 2;
    local command="$1"
    eval $command 2>&1 | while read line; do { $level $name "$line"; } done;

}

function log_file() {
    local pid="$1";
    local file="$2";

    log_debug "log_file" "Following logfile $file for program with pid: $pid";


    if [[ $verbose == 1 ]]; then
        tail -q -F --pid $pid $file;
    elif [[ $quiet == 1 ]]; then
        wait $pid;
    else
        local continue="1";
        echo "";
        while [[ $continue == 1 ]]; do
            jobke=$( jobs -l | grep " $pid ");

            if [[ -z $jobke ]]; then
                continue="0";
            else
    		    read jobid jobpid jobstatus <<<  $( echo $jobke | sed "s/\[\([0-9]*\)\]+ \([0-9]*\) \([a-zA-Z]*\).*/\1\n\2\n\3\n/" );
                log_debug "log_file" "Id / Pid / Status --> $jobid $jobpid $jobstatus";
    			if [[ $jobstatus == "Done" ]]; then
        			continue="0";
                elif [[ -n $( echo $jobstatus | grep "Exit") ]]; then
                    continue="0";
    			fi

  			fi;
            line=$( cat -v $file | sed -e "/^[ ]*$/d" -e "s|\^M|\n|g" | tail -q -n 1 );

            log_module "${line}";
  			sleep 1;
        done;
    fi;
}

function _log() {

    _date_format;       # Format the date (stored in '$_date')
    local color="$2"
    local level="$1";
    local method="$3";
    local message="$4";
    local prefix="$5";
    local format="";
    if [[ "$verbose" == "1" ]]; then
        format="$log_format_verbose";
    else
        format="$log_format";
    fi;
    local logmsg=${format//%date/$_date};
          logmsg=${logmsg//%color/$color};
          logmsg=${logmsg//%level/$level};
          logmsg=${logmsg//%message/$message};
          logmsg=${logmsg//%method/$method};
          logmsg=${logmsg//%reset/$RESET};

    _log_formatted="${prefix}${logmsg}";
}
