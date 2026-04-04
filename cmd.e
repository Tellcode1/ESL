fn main() {
  let args = sys::get_command_line_args();

  let x = args[2];
  let op = args[3];

  if (str::equal(x, "sqrt"))
  {
    if (len(args) < 4)
    {
      println("Expected argument");
      return null;
    }
    let r = math::sqrt(float(op));
    println(r);
    return null;
  }
  
  if (len(args) < 5)
  {
    println("Expected argument");
    return null;
  }

  if (str::equal(x, "pow"))
  {
    let x = float(args[3]);
    let y = float(args[4]);
    println(math::pow(x,y));
    return null;
  }


  let y = args[4];

  let r = null;
  if (str::equal(op, "+"))
  {
    r = float(x) + float(y);
  } else if (str::equal(op, "-"))
  {
    r = float(x) - float(y);
  } else if (str::equal(op, "*"))
  {
    r = float(x) * float(y);
  } else if (str::equal(op, "/"))
  {
    r = float(x) / float(y);
  }
  println(r);
  return null;
}