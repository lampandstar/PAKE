<?php
	//echo 'step '.($_POST['step']==1 ? 'one' : 'unknown');
	//echo 'Response from server : '.$_POST['data'];
	
	/**************************************************************\
	 SRP单向优化版本的优势：
		1. 减少Http-Server的维持会话的数据量
		2. 将Http-Server端的密集计算集中到一次响应，从而减少与SRP服务器的通信
						 One-Way Optimized SRP
	 C.1
		ID, a = rand(), A = g**a
						Send ID, A
								   S.1
									(s, v) = select salt, verifier from tb where id=ID
									------- Finish in SRP Server -------
									| b = rand(), B = v + g**b		   |
									| u = rand(), S = (A*v**u)**b      |
									| K = H(S)                         |
									| M2 = H(A, B, K)                  |
									------------------------------------
						Response s, u, B, M2
	 C.2
		x = H(s, P)
		S = (B - g**x)**(a+u*x)
		K = H(S)
		M1 = H(A, B, K)
		CMP(M1, M2)
						Send M1 or fail
								   S.2
									CMP(M1, M2)							
		--------------------------------------------------------
		 Server side shall maintain: M2
		
	\**************************************************************/
	
	/**
	 * HTTP server side of SRP
	 * Apache-Server的多线程模式
	 */

	session_start();
	
	#$host = $_SERVER['HTTP_HOST'];
	#$self = $_SERVER['PHP_SELF'];
	#echo 'Path : '.$path;
	
	require_once 'socket.php';
	/* Get ID */
	$id = trim($_POST['id']);
	$step = trim($_POST['step']);
	$data = trim($_POST['data']);
	assert($data != '', 'illegal message');
	$data = explode('|', $data);
	
	/* In step 1 */
	if ($step == 1)
	{
		/* look up salt & verifier with id  */
		#simulate
		$s = '0123456789';
		$v = '0123456789ABCDEF';
		#simulate
		
		$A = $data[0];
		/* send A, s, v to SRP-Server, get u,B,S */
		
		#simulate
		$u = '0123456789ABCDEFFEDCBA9876543210';
		$B = '0123456789ABCDEFFEDCBA9876543210';
		$S = '0123456789ABCDEFFEDCBA9876543210';
		#simulate
		
		#real
		/*
		if (($resp = switch_data($A.'|'.$s.'|'.$v) == null)
		{
			echo 'failed'
			exit();
		}
		$data = explode('|', $resp);
		assert(count($data) == 3, 'illegal message');
		$u = $data[0];
		$B = $data[1];
		$S = $data[2];
		*/
		#real
		
		
		/* keep M2 for next request*/
		$K = $_SESSION['key'] = hash('sha256', $S);
		$M2 = $_SESSION['M2'] = hash('sha256', $A.$B.$K);
		/* send back s|u|B|M2 */
		echo $u.'|'.$B.'|'.$K.'|'.$M2;
	}
	else if ($step == 2)
	{
		$M1 = $data[0];
		$M2 = $_SESSION['M2'];
		#echo 'M1='.$M1.'\nM2='.$M2;
		/* compare M1, M2 */
		if (strcmp($M1, $M2))
		{
			echo "failed";
			session_destroy();
			exit();
		}
		else
		{
			echo 'succeeded';
		}
	}