<?php
include("connect.php");

try {
	$conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
	$conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
	
	$id = $_POST["id"]; //current id
	$dir = $_POST["dir"];    
	
	//output data
	if ($dir == 1){//forward a frame
		$stmt =  $conn->prepare("SELECT id FROM log WHERE id > :id ORDER BY id LIMIT 1");
		$stmt->bindParam(':id', $id, PDO::PARAM_INT);
		$stmt->execute();
		$result = $stmt->fetchAll();
		if (count($result) > 0 ) {
			echo $result[0]["id"];
		}else{
			echo $id;
		}
	}elseif($dir == -1){//backwards a frame
		$stmt = $conn->prepare("SELECT id FROM log WHERE id < :id ORDER BY id DESC LIMIT 1");	
		$stmt->bindParam(':id', $id, PDO::PARAM_INT);
		$stmt->execute();
		$result = $stmt->fetchAll();
		if (count($result) > 0 ) {
			echo $result[0]["id"];
		}else{
			echo $id;
		}
	}elseif ($dir == 2){//forward to next opening event
		//forward to nearest close
		$stmt =  $conn->prepare("SELECT id FROM log WHERE id > :id AND (g_state = 0 OR c_state = 0 ) ORDER BY id LIMIT 1");
		$stmt->bindParam(':id', $id, PDO::PARAM_INT);
		$stmt->execute();
		$result = $stmt->fetchAll();
		if (count($result) > 0 ) {
			$id2 = $result[0]["id"];
			//forward to next open
			$stmt =  $conn->prepare("SELECT id FROM log WHERE id > :id AND (g_state < 0 OR c_state < 0 ) ORDER BY id LIMIT 1");
			$stmt->bindParam(':id', $id2, PDO::PARAM_INT);
			$stmt->execute();
			$result = $stmt->fetchAll();
			if (count($result) > 0 ) {
				echo $result[0]["id"];
			}else{
				echo $id;
			}
		}else{
			echo $id;
		}
	}elseif ($dir == -2){//backwards to opening event
		//forward to nearest close
		$stmt =  $conn->prepare("SELECT id FROM log WHERE id < :id AND (g_state = 0 OR c_state = 0 ) ORDER BY id LIMIT 1");
		$stmt->bindParam(':id', $id, PDO::PARAM_INT);
		$stmt->execute();
		$result = $stmt->fetchAll();
		if (count($result) > 0 ) {
			$id2 = $result[0]["id"];
			//forward to next open
			$stmt =  $conn->prepare("SELECT id FROM log WHERE id < :id AND (g_state < 0 OR c_state < 0 ) ORDER BY id LIMIT 1");
			$stmt->bindParam(':id', $id2, PDO::PARAM_INT);
			$stmt->execute();
			$result = $stmt->fetchAll();
			if (count($result) > 0 ) {
				echo $result[0]["id"];
			}else{
				echo $id;
			}
		}else{
			echo $id;
		}
	}
	
}
catch (PDOException $e) {
	print "Error: " . $e->getMessage();
}
$conn = null;

?>