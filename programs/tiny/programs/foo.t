count = 1;
n = 1;
limit = 100;

		// wtf
while (n < limit) {
    k=3;
    p=1;
    n=n+2;
    while ((k*k<=n) && (p)) { // wtf
        p=n/k*k!=n;
        k=k+2; // wtf
    }
    if (p) {
        print(n, " is prime\n");
        count = count + 1;
    }
}
// wtf
print("Total primes found: ", count, "\n");