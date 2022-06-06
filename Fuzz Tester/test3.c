int main() {
	int a1;
	int a2;
	int x = a1+a2;
	if (x > 0) {
		return x;
	}
	else {
		return x; // bug: should be ’-x’
	}
}