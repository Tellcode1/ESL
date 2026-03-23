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

fn divide_by_2(v)
{
  return v / 2;
}

fn main() {
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
    println("goob ");
  }
  else if (0) {
    println("goop");
  }
  else {
    let goob = "goob";
    println("Gambing: ", goob);
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

  file_test();

  let multi_assign_test1 = 2;
  let multi_assign_test2 = 1;
  let multi_assign_test3 = 4;
  println("Before multi assignment: ", multi_assign_test1, ", ", multi_assign_test2, ", ", multi_assign_test3);
  multi_assign_test1 = multi_assign_test2 = multi_assign_test3 = 16;
  println("After multi assignment: ", multi_assign_test1, ", ", multi_assign_test2, ", ", multi_assign_test3);

  print("For statement test (Running numbers 1 through 10): ");
  for (let i = 1; i <= 10; i++)
  {
    print(i);
    if (i != 10)
    {
      print(", ");
    }
  }
  print("\n");

  for (let i = 0; i <= 16; i++)
  {
    // divide by 8
    print(divide_by_2(divide_by_2(divide_by_2(float(i)))));
    if (i != 16-1)
    {
      print(", ");
    }
  }
  print('\n');

  let bigl = [12, 2 + 2, "Gaming", 68.0, 16, false, ["Another list!", 2 - 2.5, true], "More_stuff"];
  println(bigl);

  bigl[6] = "lmao";
  println(bigl);

  let l = [1,2,3,4,5,6,7,8,9,10];
  println(l[2]);

  println("len(", l, ") = ", len(l));

  print("[");
  for (let i = 0; i < len(l); i++)
  {
    print(l[i]);
    if (i != len(l) - 1)
    {
      print(", ");
    }
  }
  print("]");
  print("\n");

  for (let i = 0; i < len(l); i++)
  {
    l[i] *= 2;
    l[i] *= 2;
  }

  println(l);

  let sum = 0;
  for (let i = 0; i < len(l); i++)
  {
    sum += l[i];
  }
  println("Sum of all elements in the list is: ", sum);

  let INT32_MIN = -2147483648;
  let INT32_MAX = ~INT32_MIN & ~0;
  println("INT32_MAX = ", INT32_MAX);

  len(69);
}

fn file_test()
{
  let file = file_open("file", "rb");
  let size = file_size(file);
  let contents = file_read(file);
  println(contents);
  file_close(file);
}