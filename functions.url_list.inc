declare -a url_list;

## Hook to be used for this url.
declare -a url_list_hook;


## Tracks the status of a url in the list.
# -2:  No hook found for this url.
# -1:  Hook found, queued.
#  0:  Downloading.
#  1:  Finished download OK.
#  2:  Download failed.
declare -a url_list_status;


function collect_url_file() {
    local argfile="$1";
    log_debug "collect_url_file" "Reading lines from file";
    local all_ok="0";
    while read line; do
        collect_url "$line" "1";
        if [[ $? == "1" ]]; then
            all_ok="1";
        fi;
    done < <(cat "$argfile")
    return $all_ok;
}

function collect_url() {
    local arg="$1";
    local file="$2";
    local fromfile="${file:=0}";
    if [[ -z "$arg" ]]; then
        return 0;
    else
        log_debug "collect_url" "Collecting url for argument: \"$arg\"";
        if [[ -f "$arg" && "$fromfile" == "0" ]]; then
            log_debug "collect_url" " -> Argument \"$arg\" seems to be a file"
            collect_url_file "$arg";
            return $?;
        else
            if regex_map_match "$arg"; then
                local hook="$lastmatch";
                log_debug "collect_url" "Found hook for url: \"$hook\"";
                url_list.add "$arg" "$hook";
                return 0;
            else
                url_list.add "$arg" "unknown.notfound" "-2";
                return 1;
            fi;
        fi;
    fi;
}



function url_list.add() {
    local url="$1";
    local hook="$2";
    local module="${hook%.*}";
    local status="$3";
          status="${status:=-1}";
    local count="${#url_list[@]}";
    log_debug "url_list.add" "Adding url: \"$url\" Status: $status [$module]";
    url_list[$count]="$url";
    url_list_hook[$count]="$hook";
    url_list_status[$count]="$status";
    return 0;
}


function url_list.list() {
    log_info "url_list" "Listing all items.";
    local -a badlist=();
    local -a modlist=();

    for ((a=0; a < ${#url_list[@]}; a=$a+1)); do
        local hook module url status;
        hook="${url_list_hook[$a]}";
        module="${hook%.*}";
        url="${url_list[$a]}";
        status="${url_list_status[$a]}";
        if [[ "$status" == "-2" || "$status" == "2" ]]; then
            local count="${#badlist[@]}";
            badlist[$count]="$url";
        else
            if ! array_key_exists "$module" "${modlist}"; then
                local count="${#modlist[@]}";
                modlist[$count]="$module";
            fi;

            local modar="modar_$module[@]";
            local -a tmpvar="( ${!modar} )";
            local count="${#tmpvar[@]}";
            local modeval="modar_$module[$count]=\"$a\"";
            eval "local -a modar_$module";
            eval $modeval;
        fi;
    done;
    log_debug "url_list" "Badlist: ${badlist[@]}";
    if [[ "${#badlist[@]}" > 0 ]]; then
        log_warn "url_list" "Following urls failed and are not downloaded";
        for ((a=0; a < ${#badlist[@]}; a=$a+1)) do
            local failed="${badlist[$a]}";
            log_warn "url_list" "[${BRED}Failed${RESET}] --> ${failed}";
        done;
    fi;
    log_debug "url_list" "Listing hooked urls by module";
    local totalcount="${#url_list[@]}";
    totalcount="${#totalcount}";
    local printf="%0${totalcount}d";

    for ((a=0; a < ${#modlist[@]}; a=$a+1)); do
        local module="${modlist[$a]}";
        local modar="modar_$module[@]";
        local -a tmpvar="( ${!modar} )";
        log_debug "url_list" "Listing for module: [$module]";

        for ((b=0; b < ${#tmpvar[@]}; b=$b+1)); do
            local urlid="${tmpvar[$b]}";
            local url="${url_list[$urlid]}";
            local status="${url_list_status[$urlid]}";
            local statusstr="";

            if [[ $status == "-1" ]]; then
                statusstr="${BYELLOW}??$RESET";
            elif [[ $status == "0" ]]; then
                statusstr="${BBLUE}--$RESET";
            elif [[ $status == "1" ]]; then
                statusstr="${BGREEN}OK$RESET";
            elif [[ $status == "2" ]]; then
                statusstr="${BRED}!!$RESET";
            fi;

            local printfd=$( printf "$printf" "$urlid" );

            log_info "url_list" "[${BGREEN}$module${RESET}] ( $printfd ) --> [$statusstr] $url";
        done;
    done;
}

function url_list.setStatus() {
    local id="$1";
    local status="$2";
    url_list_status[$id]="$status";
}

function url_list.run() {
    local total="${#url_list[@]}";
    log_info "download" "Starting download (Total count: $total)";

    local totalcount="${#url_list[@]}";
    totalcount="${#totalcount}";
    local printf="%${totalcount}d";

    local count_ok="0";
    local count_failed="0";

    for ((d=0; $d<$total; d=$d+1)); do
        local urlid=$d;
        let "urlid++";
        local url="${url_list[$d]}";
        local hook="${url_list_hook[$d]}";
        local status="${url_list_status[$d]}";
        local module="${hook%.*}";

        local id=$( printf "$printf" "$urlid" );
        local ok=$( printf "$printf" "$count_ok" );
        local fail=$( printf "$printf" "$count_failed" );

        log_debug "download" "Queue information:";
        log_debug "download" " --> Done: $urlid / Total: $total";
        log_debug "download" " --> Ok: $ok / Failed: $fail";

        log_debug "download" "URL information:";
        log_debug "download" " --> Hook: $hook";
        log_debug "download" " --> Status: $status";

        if [[ "$status" == "-2" ]]; then
            log_debug "download" "[ $id / $total ] ${BRED}ERR$RESET: $WHITE$url$RESET";
            url_list.setStatus "$d" "2";
            let "count_failed++";
            return 1;
        fi;

        log_info "download" "[ $id / $total ] URL: $WHITE$url$RESET";
        log_debug "download" "--> Running hook $hook";
        url_list.setStatus "$d" "0";
        modulelog_prefix="[ $id / $total ] ";
        modulelog_module="$module";

        if run_hook "$hook" "$url"; then
            url_list.setStatus "$d" "1";
            let "count_ok++";

        else
            url_list.setStatus "$d" "2";
            let "count_failed++";
        fi;
    done;

}
