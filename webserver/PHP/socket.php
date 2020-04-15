<!--
  This is a simple socket client, sending message to a socket server
 -->
<?php

	/* IP:port of target server */
	$server_ip="10.10.81.116";
	//$server_ip="127.0.0.1";
	$port = 8080;

	/* response http header */
	header("Content-type: text/html; charset=utf-8");

	/* unify character encoding as UTF-8 */
	function doEncoding($str)
	{
        $encode = strtoupper(mb_detect_encoding($str, ["ASCII",'UTF-8',"GB2312","GBK",'BIG5']));
        if($encode!='UTF-8'){
            $str = mb_convert_encoding($str, 'UTF-8', $encode);
        }
        return $str;
    }
	
	echo "<center><h1>Socket</h1></center>";
	
	if (($sock = socket_create(AF_INET, SOCK_STREAM, 0)) == false)
	{
		echo "Socket create failed, details : ".doEncoding(socket_strerror(socket_last_error()));
	}
	
	if (!socket_connect($sock, $server_ip, $port))
	{
		echo "Socket connect failed, details : ".doEncoding(socket_strerror(socket_last_error($sock)));
	}

	echo "socket established.";

	$buf = "hello, how are you!";

	//if(!socket_sendto($sock, $buf, strlen($buf), 0, $server_ip, $port))
	if (($written =socket_write($sock, $buf)) == false)
	{
		echo "Socket write failed, details : ".doEncoding(socket_strerror(socket_last_error($sock)));
		socket_close($sock);
		exit();
	}
	else
	{
		echo $written."B written.";
	}
	
	echo "msg sent.";

	$buf="";
	$msg="";

	/*
	if(!socket_recvfrom($sock, $msg, 256, 0, $msg, $port))
	{
		echo "recvieve error!";
		socket_close($sock);
		exit();
	}
	*/
	$msg = socket_read($sock, 2048);
	echo trim($msg)." ";

	socket_close($sock);
?>