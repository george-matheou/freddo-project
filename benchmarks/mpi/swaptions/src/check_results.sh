	# Check the FREDDO Results
	# || means: if the first command succeed the second will never be executed
	cmp --silent out/freddo.out out/serial.out || echo "!!!!!!!!!!!!!!!!!!!!!!!! FREDDO Wrong Results"
