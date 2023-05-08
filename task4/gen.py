size = 1000

with open("nums", "w") as file:
	for i in range(size):
		if (i + 1) == 500: 
			file.write("-124750\n")
		elif (i + 1) == 1000:
			file.write("-374250\n")
		else:
			file.write(f"{i + 1}\n")