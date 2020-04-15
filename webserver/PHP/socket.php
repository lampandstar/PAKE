<?php
	header("Content-type: text/html; charset=utf-8");

	function doEncoding($str)
	{
        $encode = strtoupper(mb_detect_encoding($str, ["ASCII",'UTF-8',"GB2312","GBK",'BIG5']));
        if($encode!='UTF-8'){
            $str = mb_convert_encoding($str, 'UTF-8', $encode);
        }
        return $str;
    }
	/* address of SRP server */
	$server_ip="10.10.81.116";
	//$server_ip="127.0.0.1";
	$server_port = 8080;
	
	function set_addr($ip, $port)
	{
		$server_ip = $ip;
		$server_port = $port;
	}
	
	/* Send data to SRP server and get response data */
	function switch_data($msg)
	{
		/* create socket */
		if (($sock = socket_create(AF_INET, SOCK_STREAM, 0)) == false)
		{
			echo "Socket create failed, details : ".doEncoding(socket_strerror(socket_last_error()));
			return null;
		}
		/* connect */
		if (!socket_connect($sock, $server_ip, $server_port))
		{
			echo "Socket connect failed, details : ".doEncoding(socket_strerror(socket_last_error($sock)));
			return null;
		}
		#socket established
		/* send message */
		//if(!socket_sendto($sock, $buf, strlen($buf), 0, $server_ip, $port))
		if (($written = socket_write($sock, $msg)) == false)
		{
			echo "Socket write failed, details : ".doEncoding(socket_strerror(socket_last_error($sock)));
			socket_close($sock);
			return null;
		}
		else
		{
			#echo $written."B written.";
			/* receive results ending with '\r', '\n' or '\0' */
			return socket_read($sock, 1024, PHP_NORMAL_READ);
		}
		/* close socket */
		socket_close($sock);
	}
?>