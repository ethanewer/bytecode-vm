## Installation
- Clone or download this repository
- Once inside the directory enter the command `make` in the terminal
- After that, you use the repo with `./main` or run a script with `./main script.txt`


## Hello World
    # comments begin with '#'
    println('Hello, World!');

## Variables
    let n = 2;
    let b = true;
    let s = 'bagel';

## While Loops
    let n = 5;
    let n_factorial = 1;
    
    while (n > 1) {
  	  n_factorial *= n;
  	  n--;
  	}
    
    println(n_factorial); # 120

## For Loops
    let n = 5;
    let n_factorial = 1;
    
    for (let i = 1; i <= n; i++) {
      n_factorial *= i;
    }
    
    println(n_factorial); # 120

## Functions and Closures
    fn fib(n) {
      if (n < 2) return 1;
      return fib(n - 2) + fib(n - 1);  
    }
    
    println(fib(10)); # 89
    
    fn make_counter() {
      let i = 0;
      return fn() {
        i++;
        return i;
      };
    }
    
    let counter = make_counter();
    
    println(counter()); # 1
    println(counter()); # 2
    println(counter()); # 3

## List Class
    let list = List();
    
    for (let i = 0; i < 5; i++) {
      list.push(i);
    }
    
    println(list.to_string()); # [0, 1, 2, 3, 4]
    let doubled_list = list.map(fn(x) { return 2 * x; });
    
    println(doubled_list.to_string()); # [0, 2, 4, 6, 8]
    let filtered_list = list.filter(fn(x) { return x > 2; });
    
    println(filtered_list.to_string()); # [3, 4]
    
    let sum = list.reduce(fn(a, b) { return a + b; }, 0);
    println(sum); # 10
    
    println(list[2]); # 2
    
    list[2] = 11;
    println(list[2]); # 11

## Set Class
    let A = Set();
    for (let i = 1; i < 10; i += 2) {
      A.add(i);
    }
    
    let B = Set();
    for (let i = 0; i < 5; i++) {
      B.add(i);
    }
    
    println(A.to_string(), B.to_string()); # {1, 3, 5, 7, 9} {0, 1, 2, 3, 4}
    
    let union = A.union(B); 
    println(union.to_string()); # {1, 3, 5, 7, 9, 0, 2, 4}
    
    let intersect = A.intersect(B);
    println(intersect.to_string()); # {1, 3}

## Map Class
    let map = Map();
    
    for (let i = 0; i < 5; i++) {
      map['key_' + string(i)] = i;
    }
    
    println(map.to_string()); # {key_0: 0, key_1: 1, key_2: 2, key_3: 3, key_4: 4}
    
    map['key_0'] = true;
    map['key_3'] = 'bagel';
    println(map.to_string()); # {key_0: true, key_1: 1, key_2: 2, key_3: bagel, key_4: 4}

## User Defined Classes and Inheritance
    class Animal {
      Animal(name) {
        this.name = name;
      }
      
      say_name() {
        println(this.name);
      }
    }
    
    let animal = Animal('Eli');
    animal.say_name(); # Eli
    
    class Cat : Animal {
      Cat(name) {
        this.name = name;
      }
      
      meow() {
        println('Meow!');
      }
    }
    
    let cat = Cat('Nick');
    cat.say_name(); # Nick
    cat.meow(); # Meow!

