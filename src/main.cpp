#include <iostream>

#include "scanner.hpp"

int main()
{
  std::string_view s{
      R"(func main(fs_in: FsInput): color4 {
                      let color = texture(floor_texture, fs_in.tex_coords).rgb;
                      // ambient
                      let ambient = 0.05 * color;
                      // diffuse
                      let light_dir = normalize(light_pos - fs_in.frag_pos);
                      let normal = normalize(fs_in.normal);
                      let diff = max(dot(light_dir, normal), 0.0);
                      let diffuse = diff * color;
                      // specular
                      let view_dir = normalize(view_pos - fs_in.frag_pos);
                      let halfway_dir = normalize(light_dir + view_dir);
                      let spec = pow(max(dot(normal, halfway_dir), 0.0), 32.0);
                      let specular = color3(0.3) * spec;
                      (ambient + diffuse + specular, 1.0)
                  })"};
  beyond::Scanner scanner{std::string{s}};

  for (const auto token : scanner) {
    std::cout << token << '\n';
  }
}
