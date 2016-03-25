<?php
include("connect.php");

try {
	$conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
	$conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
	
	$time = intval($_POST["time"]);
	if ($time == 0) {
		$time = time();
	}
	
	$stmt = $conn->prepare("SELECT id,DATE_FORMAT(timestamp,'%r') as time FROM log WHERE timestamp <= FROM_UNIXTIME(:time) AND keyframe = 1 ORDER BY id DESC LIMIT 100");
	$stmt->bindParam(':time', $time, PDO::PARAM_INT);
	$stmt->execute();
	$result = $stmt->fetchAll();

	//request_data()
	if (count($result) == 0) {
		echo "No Results Found<br>";
	}
	foreach ($result as $item){
    echo "<a onclick=id_load(" . $item['id'] . ")>" . $item['time'] . "</a><br>";
	}
	echo "<br>";
	
}
catch (PDOException $e) {
	print "Error: " . $e->getMessage();
}
$conn = null;

?>