struct vec2
{
  let x;
  let y;
};

fn v2add(va, vb)
{
  return vec2(va.x + vb.x, va.y + vb.y);
}

fn v2scale(v, f)
{
  return vec2( v.x * f, v.y * f );
}

fn main()
{
  let pos = vec2(0.0, 1.0);
  let vel = vec2(1.0, 0.0);
  
  let r = v2add(pos, vel);
  println(v2add(r, vel));

  println(v2scale(r, 10.0));
}