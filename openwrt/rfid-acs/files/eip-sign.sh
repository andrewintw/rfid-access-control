#!/bin/bash

# No need to modify
encodeURL=''
parmNum="$#"
cmd="$0"
parm1="$1"
parm2="$2"
cookieC=0
cookieFile=`tempfile`

today=`date +%Y-%m-%d`
now_day=`date -d $today +%d`
dayofweek=`date -d $today +%u`
is_use_hday_file=0
is_working_day=1

is_force=0
diffTime=0
scriptTime="$((`date +%s%N`/1000000))"


# download file from https://data.gov.tw/dataset/14718
# Put the file in the same path as eip-sign.sh
tw_hday_file="dgpa.gov-108.csv"


# Change for your login info
userID='andrew.lin'
userPSW='108008'

get_file_path() {
	local file="$0"
	local path=$(cd `dirname $file `; pwd)
	local filename=`basename $file`
	local path2file="$path/$filename"

	if [ -f "$path2file" ]; then
		echo $path2file
	else
		echo "/path/to/$filename"
	fi
}

usage() {
	cat << EOF

Usage: $cmd  <in|out|ck>  [-f]
-------------------------------------------------------------------
       parm1: (required)
         in : sign-IN  -- Only ONCE a day! Will be ignored after the first time
         out: sign-OUT -- Accept multiple times a day. Will accepts the latest submit
         ck : get session/cookie info

       parm2: (optional)
          -f: do sign no matter it is a working day or not
              (in default, the sign will be ignored in weekend)


Example:
-------------------------------------------------------------------
       $0 out     # sign-out in working day
       $0 in -f   # force sign-in in weekend


Cron line explaination:
-------------------------------------------------------------------
* * * * * "command to be executed"
| | | | |
| | | | |
| | | | +---- Day of the Week   (range: 1-7, 1 standing for Monday)
| | | +------ Month of the Year (range: 1-12)
| | +-------- Day of the Month  (range: 1-31)
| +---------- Hour              (range: 0-23)
+------------ Minute            (range: 0-59)

Ref: https://crontab.guru

Example: (edit by "crontab -e" command)

    # 9:30~9:55AM random sign-in, 7:00~7:25PM random sign-out

    30 9 * * 1-5 sleep \`shuf -i 0-1500 -n 1\`; `get_file_path $cmd` in
    0 19 * * 1-5 sleep \`shuf -i 0-1500 -n 1\`; `get_file_path $cmd` out

EOF
}

chk_parm1() {
	local action=`echo ${parm1::1} | tr [A-Z] [a-z]`
	case "$action" in
		"i")	;;
		"o")	;;
		"c")	;;
		*)
			usage && exit 1
	esac
}

chk_parm2() {
	if [ "$parm2" = "-f" ]; then
		is_force=1
	else
		usage && exit 1
	fi
}

get_diffTime() {
	# in javascript:
	# //取得本地端之時間
	# var scriptTime=(new Date()).getTime();
	# //取得伺服器端時間差
	# var diffTime=1565261653583-scriptTime;	// 1565261653583 is serverTime

	local serverTime=`\
	curl -s 'http://eip.browan.net/login.jsp' \
	-H 'Connection: keep-alive' \
	-H 'Pragma: no-cache' \
	-H 'Cache-Control: no-cache' \
	--compressed --insecure | \
	grep "var\ *diffTime" | awk -F '=' '{print $2}' | awk -F '-' '{print $1}' | grep -oh [0-9]*`

	diffTime=$((${serverTime} - ${scriptTime}))
}

do_init() {
	if [ "$parmNum" -lt 1 ]; then usage && exit 1; fi

	if [ "$parmNum" = "2" ];then
		chk_parm2
		chk_parm1
	fi

	if [ "$parmNum" = "1" ]; then
		chk_parm1
	fi

	rm -rf $cookieFile
}

update_env() {
	local path=$(cd `dirname $cmd `; pwd)
	if [ -f $path/$tw_hday_file ]; then
		tw_hday_file="$path/$tw_hday_file"
		echo "INFO>> use Taiwan Government Agencies Holiday Calendar: $tw_hday_file"
		is_use_hday_file=1
	else
		echo "INFO>> cannot find Calendar file: $tw_hday_file"
		is_use_hday_file=0
	fi
}

parseInt() {
	printf "%d\n" \'$1
}

# Encoded URL: 574@545@551@548@555@565@626@637@631@609@630@612@573@639@634@637@565@546@547@555@547@547@555@565@546@565@546@550@549@551@554@549@554@555@545@544@555@545@551
# Decoded URL: -2478&andrew.lin&108008&1&1564969823824
encode() {
	local url="$1"
	local i=0
	local data=''
	for c in `echo $url | sed -e 's/\(.\)/\1\n/g'`; do 
		i=$(($i+1))
		encstr=$(printf '%#d@' "$((`parseInt $c`^531))")
		data="${data}${encstr}"
	done
	echo "${data::-1}"
}

get_jsession_id() {
	if [ -f "$cookieFile" ]; then
		grep JSESSIONID $cookieFile | awk '{ print $NF }'
	fi
}

show_sign_record() {
	local sessionID=`get_jsession_id`
	local resp=`tempfile`

	curl -s 'http://eip.browan.net/WorkSign/signList.jsp?SHOW_ID=2' \
		-H 'Connection: keep-alive'  \
		-H 'Pragma: no-cache'  \
		-H 'Cache-Control: no-cache'  \
		-H 'Upgrade-Insecure-Requests: 1'  \
		-H 'User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.142 Safari/537.36'  \
		-H 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3'  \
		-H 'Referer: http://eip.browan.net/common/menu.jsp'  \
		-H 'Accept-Encoding: gzip, deflate'  \
		-H 'Accept-Language: zh-TW,zh;q=0.9,en-US;q=0.8,en;q=0.7,ja;q=0.6,zh-CN;q=0.5'  \
		-H "Cookie: JSESSIONID=$sessionID" \
		--compressed --insecure \
		-o $resp

	local signInTime=`grep -a -A13 $today $resp | sed '4!d' | tr -d '' | tr -d '\t' | sed -e 's/<[^>]*>//g' | iconv -f big5 -t utf8`
	local signOtTime=`grep -a -A13 $today $resp | sed '8!d' | tr -d '' | tr -d '\t' | sed -e 's/<[^>]*>//g' | iconv -f big5 -t utf8`

	cat <<EOF
------------------
date: $today
 in : $signInTime
 out: $signOtTime

EOF
	rm -rf $resp
}

login() {
	local urlData="${diffTime}&${userID}&${userPSW}&${cookieC}&${scriptTime}"
	urlData=`encode ${urlData}`
	encodeURL="http://eip.browan.net/check.jsp?urlData=${urlData}"

	curl -c $cookieFile "$encodeURL"

	local action=`echo ${parm1::1} | tr [A-Z] [a-z]`
	if [ "$action" = "c" ]; then
		cat <<EOF
encodeURL : $encodeURL
jSessionID: `get_jsession_id`
EOF
		show_sign_record
		exit 0
	fi
}

curl_sign() {
	local action="$1"

	if [ "$action" = "in" ]; then
		local FUN_ID=1
	else
		local FUN_ID=2
	fi

	local sessionID=`get_jsession_id`

	local resp=`\
		curl -s \
		'http://eip.browan.net/WorkSign/signCtrl.jsp' \
		-H 'Connection: keep-alive' \
		-H 'Pragma: no-cache' \
		-H 'Cache-Control: no-cache' \
		-H 'Origin: http://eip.browan.net' \
		-H 'Upgrade-Insecure-Requests: 1' \
		-H 'Content-Type: application/x-www-form-urlencoded' \
		-H 'User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.142 Safari/537.36' \
		-H 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3' \
		-H 'Referer: http://eip.browan.net/common/default.jsp' \
		-H 'Accept-Encoding: gzip, deflate' \
		-H 'Accept-Language: zh-TW,zh;q=0.9,en-US;q=0.8,en;q=0.7,ja;q=0.6,zh-CN;q=0.5' \
		-H "Cookie: JSESSIONID=$sessionID" \
		--data "memid=&SHOW_ID=2&FUN_ID=$FUN_ID" \
		--compressed --insecure`

		#-H 'Cookie: JSESSIONID=9A22241D8D23AD2F905605E5EC51A26B' # easy way is use `curl -b $cookieFile ...`

	local is_err=`echo "$resp" | grep 'Error report'`
	if [ "$is_err" != "" ]; then
		echo "CURL>> fail to sign-$action"
		rm -rf $cookieFile
		exit 1
	fi
}

do_sign_out() {
	curl_sign "out"
}

do_sign_in() {
	curl_sign "in"
}

do_sign() {
	local action=`echo ${parm1::1} | tr [A-Z] [a-z]`
	case "$action" in
		"i")
			do_sign_in
			;;
		"o")
			do_sign_out
			;;
		*)
			usage && exit 1
	esac
}

do_done () {
	cat <<EOF

jSessionID: `get_jsession_id`
      CURL: sign OK, check your submit at the following URL:
            $encodeURL
`show_sign_record`

EOF
	rm -rf $cookieFile
}

list_working_days() {
    ncal -M -h | sed -n '2,6p' | sed "s/[[:alpha:]]//g" | fmt -w 1 | sort -n | tr -d ' '
}

is_tw_holliday() {
	local hday=$(iconv -f big5 -t utf8 $tw_hday_file | grep `date -d "$today" +%Y%m%d` | head -n 1 | awk -F ',' '{print $3}')
	if [ "$hday" = "2" ]; then
		# holliday
		echo 1
	else
		# weekday
		echo 0
	fi
}

wdays_chk() {
	#for d in `list_working_days`; do
	#	wd=`printf "%02d" $d`
	#	if [ "$now_day" = "$wd" ]; then
	#		#echo "Today($now_day) is working day"
	#		is_working_day=1
	#		break
	#	fi
	#done

	if [ $dayofweek -ge 6 ]; then
		is_working_day=0
		echo "Today($now_day) is weekend"
	fi


	if [ "$is_use_hday_file" = "1" ]; then
		if [ `is_tw_holliday` -eq 1 ]; then
			is_working_day=0
			echo "Today($today) is holliday"
		fi
	fi

	if [ "$is_working_day" = "0" ]; then
		if [ "$is_force" != "1" ]; then
			exit
		fi
	fi
}

do_main() {
	do_init && \
	update_env && \
	get_diffTime && \
	wdays_chk && \
	login && \
	do_sign && \
	do_done
}

do_main
