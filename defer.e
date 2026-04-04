let ratio = 0;

fn open()
{
  ratio++;
  return io::open("file", "r");
}

fn close(fd)
{
  ratio--;
  io::close(fd);
}

fn main()
{
  for (let i = 0; i < 1000; i++)
  {
    let fd = open();
    defer close(fd);

    // oo bracketed defers oo
    defer {
      let fd2 = open();
      close(fd2);
    };
  }
  println(ratio, " Files were leaked");
  println("If the number above is greater than 0, it indicates a major issue during compilation! Please report it to my github.");
}