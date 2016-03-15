<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        
    $stmt = $conn->prepare("SELECT * FROM log ORDER BY id DESC LIMIT 1");
    $stmt->execute();
    $result = $stmt->fetchAll();

    if (count($result) > 0 ) {
		$row = $result[0];
		
		//if data is stale, supress showing the last database entery
		if (time() - strtotime($row["timestamp"]) > 30 ){		
			$row["c_live"] = 0;
			$row["g_live"] = 0;
			$row["g_lag"] = 0; 
			$row["g_volts"] = 0; 
			$row["g_temp1"]= 0; 
			$row["g_temp2"]= 0; 
			$row["c_live"] = 0; 
			$row["c_lag"] = 0; 
			$row["g_volts"] = 0; 	
			$row["c_temp1"]= 0;  
			$row["c_temp2"]= 0; 
		}
		
		//lookup when gordon was last seen if not online
		if ($row["g_live"] != 1){
			$stmt = $conn->prepare("SELECT timestamp FROM log WHERE g_live = 1 ORDER BY id DESC LIMIT 1");
			$stmt->execute();
			$temp = $stmt->fetchAll();
			$g_time = $temp[0]["timestamp"];
		}else{
			$g_time = $row["timestamp"];
		}
		
		//lookup when chell was last seen if not online
		if ($row["c_live"] != 1){
			$stmt = $conn->prepare("SELECT timestamp FROM log WHERE c_live = 1 ORDER BY id DESC LIMIT 1");
			$stmt->execute();
			$temp = $stmt->fetchAll();
			$c_time = $temp[0]["timestamp"];
		}else{
			$c_time = $row["timestamp"];
		}
		
		//output data
		echo $row["id"]. "\t" . $row["synced"]. "\t" . 
		
		$row["g_live"]. "\t" . $g_time . "\t" .
		$row["g_lag"]. "\t" . $row["g_volts"]."\t" . 
		$row["g_temp1"]. "\t" . $row["g_temp2"]."\t" .
		
		$row["c_live"]. "\t" . $c_time . "\t" .
		$row["c_lag"]. "\t" . $row["g_volts"]."\t" . 		
		$row["c_temp1"]. "\t" . $row["c_temp2"];
		
    }
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>