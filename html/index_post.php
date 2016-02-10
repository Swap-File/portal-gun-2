
<?php 
$file = fopen("/tmp/FIFO_PIPE","w") or die("check owner of FIFO_PIPE and make sure it's already created!");
echo fwrite($file, $_POST["mode"] . " " . $_POST["changeto"] . "\n");
fflush ($file);
fclose ($file);
?>