#!/usr/bin/awk -f

BEGIN {
	FS = ","
	rows = 0
	cols = 0
	if (ARGC < 2) { exit; }
	name = ARGV[1]
	for (i = 1; i < ARGC - 1; i++)
	{
		ARGV[i] = ARGV[i + 1]
	}
	ARGC--
	delete ARGV[ARGC]
}

/^$/ {next;}
{
	sub(",$","")
}
{
	for (i = 1; i <= NF; i++) {
		base = (i - 1) "," rows
		if (0 == $i) { continue }
		layout[base] = $i - 1
	}
	rows++
	cols = (NF > cols) ? NF : cols
}
END {
	bpc = int((rows - 1)/8 + 1)
	printf "unsigned char const %s[] = {\n", name
	printf "\t%d,\n", bpc
	for (i = 0; i < cols; i++) {
		x = 0
		a = 0
		for (j = 0; j < rows; j++) {
			base = i "," j
			if (base in layout) {
				x += 2^j
				v[a] = layout[base]
				a++
			}
		}
		printf "\t"
		for (j = 0; j < bpc; j++) {
			printf "0x%02X,", x % 256
			x = int(x / 256)
		}
		for (j = 0; j < a; j++) {
			printf "%3d,", v[j]
		}
		printf "\n"
	}
	print "};"
}
