fn PISS()
{
  println("PISS!");
}

fn WHATS2PLUS2()
{
  println("2 + 2 is ", 2 + 2);
}

fn TakesIn2Arguments(x, y)
{
  return x * y;
}

fn different_return_types(flag)
{
  if (flag) {
    return "True!";
  } else {
    return 0;
  }
}

fn never_return_because_im_evil()
{
  while (true) {}
}

fn update()
{
  println("Running 100 iterations of free fall test (Results are for after 100 seconds):");

  let position = 0;
  let velocity = 0;
  let acceleration = 9.8;

  let i = 0;
  while (i < 100)
  {
    let tmp_v = velocity + acceleration;
    position = position + velocity;
    velocity = tmp_v;
    i++;
  }

  println("position = ", position, " meters");
  println("velocity = ", velocity, " meters/sec");
}

println("Im gaming rn");
println(32);
println(16.0);
println(false);

println(5 * 2);
println(5 + 2);
println(5 / 2);
println(5.0 / 2.0);

println(!!!!!!!!!!true);

println(10 > 2);
println(10 < 2);
println(10 % 4);

if (3 > 2) {
  println("Scientific breakthrough: 3 is greater than 2");
} else {
  println("2nd class maths fail :3");
}

if ((10 - 2) < 0) {
  println("PISS");
} else if ((10 - 2) > 0) {
  println("BIG PISS");
} else {
  println("Secret message");
}

if (0) {
  println("goob");
}
else if (0) {
  println("goop");
}
else {
  println("Gambing");
}

let x = 16;
let y = 32;
println(x);

x = 32 + y;
println(x);

let z = x + y - 90;
while (z >= 0)
{
  print(z);
  if (z != 0)
  {
    print(", ");
  }
  z--;
}
print("\n");

println("16 ^ 32 = ", 16 ^ 32);
println("16 | 32 = ", 16 | 32);
println("3 & 2 = ", 3 & 2);

while (false) {} while (false) {}
while (false) {} while (false) {}
while (false) {} while (false) {}
while (false) {} while (false) {}
while (false) {} while (false) {}

if (false)
{

}
println("Shouldn't be skipped ^-^"); // there was bug where jump would set the instruction index
                                     // and then the loop would iterate (instruction + 1), skipping an instruction.

PISS();

WHATS2PLUS2();

println(TakesIn2Arguments(10, 20));

println("Same function, multiple return types: ", different_return_types(true), ", ", different_return_types(0));

let coercion_test = 2 + 2.5;
println(coercion_test);

let it_did_not_work = 3.2 + 5.2;
println(it_did_not_work);

let why_does_it_work_now = 6.2 + true;
println(why_does_it_work_now);

let NO_LEAKS_YAYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY = 68;
println(NO_LEAKS_YAYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY, " ", NO_LEAKS_YAYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY, " ", NO_LEAKS_YAYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY);

update();

// Unlike C, functions can be called before they are declared!
call_before_declare();
fn call_before_declare()
{
  println("This function was called before it was declared!");
}

say_hello_from_c();

let multi_assign_test1 = 2;
let multi_assign_test2 = 1;
let multi_assign_test3 = 4;
println("Before multi assignment: ", multi_assign_test1, ", ", multi_assign_test2, ", ", multi_assign_test3);
multi_assign_test1 = multi_assign_test2 = multi_assign_test3 = 16;
println("After multi assignment: ", multi_assign_test1, ", ", multi_assign_test2, ", ", multi_assign_test3);