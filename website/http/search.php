<?php
include("connect.php");

try {
	$conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
	$conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
	
	$time = intval($_POST["time"]);
	if ($time == 0) {
		$time = time();
	}
	
	$stmt = $conn->prepare("SELECT id,timestamp FROM log WHERE timestamp <= FROM_UNIXTIME(:time) AND keyframe = 1 ORDER BY id DESC LIMIT 100");
	$stmt->bindParam(':time', $time, PDO::PARAM_INT);
	$stmt->execute();
	$result = $stmt->fetchAll();
	
	//request_data()
	
	foreach ($result as $item) {
    echo "<a onclick=id_load(" . strval($item['id']) . ")>" . $item['timestamp'] . "</a><br>";
	}
	
}
catch (PDOException $e) {
	print "Error: " . $e->getMessage();
}
$conn = null;

?>