fn main(args)
{
  if (list::len(args) == 0) return;

  for (let i = 0; i < list::len(args); i++)
  {
    let fd = io::open(args[i], "r");
    if (fd == null)
    {
      println("cat: Failed to open file: ", args[i]);
    }

    defer io::close(fd);

    let s = null;
    while (s = io::readln(fd))
    {
      print(s);
    }
  }
}