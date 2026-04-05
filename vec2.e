struct vec2
{
  let x;
  let y;
};

struct vec4
{
  let x;
  let y;
  let z;
  let w;
};

struct mat4
{
  let rows0;
  let rows1;
  let rows2;
  let rows3;
};

fn v2add(va, vb)
{
  return vec2(va.x + vb.x, va.y + vb.y);
}

fn v2sub(va, vb)
{
  return vec2(va.x - vb.x, va.y - vb.y);
}

fn v2scale(v, f)
{
  return vec2( v.x * f, v.y * f );
}

fn m4identity()
{
  return mat4(
    vec4(1.0, 0.0, 0.0, 0.0),
    vec4(0.0, 1.0, 0.0, 0.0),
    vec4(0.0, 0.0, 1.0, 0.0),
    vec4(0.0, 0.0, 0.0, 1.0),
  );
}

fn main()
{
  let pos = vec2(0.0, 1.0);
  let vel = vec2(1.0, 0.0);
  
  let r = v2add(pos, vel);
  println(v2add(r, vel));

  r.x = r.y = 0.1;

  println(v2scale(r, 10.0));

  println(m4identity());
}