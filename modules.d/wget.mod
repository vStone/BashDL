## Module information
# Info="Basic file getting with wget.";
# Author="Po0ky <Jan@buitendezone.be>";
# Homepage="http://www.buitendezone.be/";

wget_prerequisites=( "wget" );
wget_config=( "_extra_opts" );
wget_default=( "_" );
wget_dep=( );


wget_regex[0]="http://.*";
wget_hook[0]="url";


function wget.url() {
    local url="$1";
    log_module "Fetching url: $*";
    local domain=$()

    ## WGET PROBLEMs
    # If there is no output file specified in wget,
    #  it might prove difficult to check what file was downloaded.
    #
    # > So we first try to get the basename of the url.
    #   > Check if the basename exists. Otherwise prompt or overwrite? -y overwrites.
    #   > If this fails, we'll output
    #     all files to a directory. Probably something like domain-XXHASHXX.
    #     > Check if the directory allready exists. Have to see what the features of
    #       mktemp are for this part.
    #     > If there is only one file downloaded without a basename given,
    #       we might as well just rename to that file.
    #       > Check if filename exists. otherwise keep folder name.


    local domain=$( echo $url | sed -e "s|^[a-z]*://||g" -e "s|\([^/]*\).*|\1|" );
    local protocol=$( echo $url | grep "^[a-z]*://" | sed -e "s|^\([a-z]*\)://\(.*\)|\1|" );
          protocol="${protocol:=http}";
    local path=$( echo $url | sed -e "s|^[a-z]*://||g"  -e "s|\([^/]*\)\(.*\)|\2|" );
          path="${path:=/}";
    local basename=$( basename $path 2>&1 );

    local dirname=$( echo ${basename} | sed "s|\.||g" );
#          dirname="${dirname}.XXXXXX";

    log_debug "wget.url" "Domainname:  $domain";
    log_debug "wget.url" "Protocol:    $protocol";
    log_debug "wget.url" "Path:        $path";
    log_debug "wget.url" "Basename:    $basename";
    log_debug "wget.url" "Dirname:     $dirname";

    local output="";
    ## If there is already a file like this, try to continue it
    ## Same for a directory with the name.
    if [[ -f $basename || -d $basename ]]; then
        _wget_log $url $wget_extra_opts --continue --no-host-directories;
        retval=$?;
        output="$basename";
    else
        # Create a directory to dump wget to.
        maketempd "$dirname" || return 1;
        local tmpdir="$tmp_dir";
        _wget_log $url $wget_extra_opts --continue --no-host-directories --directory-prefix=$tmpdir;
        retval=$?;

        local count="0";
        local onefile="";
        for file in $( ls -c1 $tmpdir); do
            onefile="$file";
            if [[ -n $onefile ]]; then
                let "count++";
                onefile="$tmpdir/$onefile";
            fi;
        done;
        if [[ $count == 0 ]]; then
            output="";
        elif [[ $count == 1 ]]; then
            local tmpoutput=$( basename $onefile );
            output="$tmpdir/$tmpoutput";
        elif [[ $output > 1 ]]; then
            output="$tmpdir";
        fi;
    fi;

    if [[ $retval == 0 ]]; then
       log_info "wget.url" "Successfully downloaded";
       log_debug "wget.url" "Output: $output";
        _last_downloaded_file="$output";
        return 0;
    else
        log_warn "wget.url" "Download failed with EXIT CODE $retval";
        _last_downloaded_file="";
        return 1;
    fi;
}

# Just get the file (for further processing);
function _wget_dl() {
    local url="$1";
    shift;
    local arguments="$*";
    log_debug "_wget_dl" "Arguments: $BGREEN$arguments$RESET";
    log_debug "_wget_dl" "Url: $url";
    wget $arguments "$url" 2>&1
}

# Download and log the output.
function _wget_log() {
    local url="$1";
    shift;
    local arguments="$*";
    # Log file for wget.
    if maketemp; then
        local tmpfile="$tmp_file";
    else
        return 1;
    fi;

    arguments="${arguments} --progress=bar:force -o $tmpfile";
    log_debug "_wget_log" "Arguments: $BGREEN$arguments$RESET";
    log_debug "_wget_dl" "Url: $url";

    wget $arguments "$url" 2>&1 &
    pid=$!;
    add_pid $pid;
    log_file "$pid" "$tmpfile";
    wait $pid;
    retval=$?;
    del_pid $pid;
    return $retval;
}
