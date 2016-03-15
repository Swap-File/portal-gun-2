<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    $stmt = $conn->prepare("INSERT INTO log (image,ip) VALUES (:image,:ip)");

	var_dump( $_SERVER['REMOTE_ADDR']) ;
	var_dump( $_FILES["img"]) ;
	
	$imageData = file_get_contents($_FILES["img"]["tmp_name"]);
	$textData = explode(",",file_get_contents($_FILES["csv"]["tmp_name"]));
	
	var_dump($textData);
	
	
    $stmt->bindParam(':image', $imageData, PDO::PARAM_LOB);
	
	$stmt->bindParam(':ip', $_SERVER['REMOTE_ADDR'], PDO::PARAM_STR);
	

    $stmt->execute();
			
			

    echo 'done' ;
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>