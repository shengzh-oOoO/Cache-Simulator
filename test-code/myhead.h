void print_s(char *str){
	asm(
		"li a7, 0;"
		"scall"
	);
}
void print_c(char c){
	asm(
		"li a7, 1;"
		"scall"
	);
}
void print_d(int d){
	asm(
		"li a7, 2;"
		"scall"
	);
}
void scan_d(){
	asm(
		"li a7, 3;"
		"scall"
	);
}
void my_exit(){
	asm(
		"li a7, 4;"
		"scall"
	);
}
void time(){
	asm(
		"li a7, 5;"
		"scall"
	);
}