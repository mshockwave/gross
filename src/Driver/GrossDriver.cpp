#include <cxxopts.hpp>
#include <iostream>

static void InitializeCLIOptions(cxxopts::Options& Opts) {
  Opts.add_options()
    ("v,version", "Show version")
    ("h,help", "Show this message");
}

int main(int argc, char** argv) {
  cxxopts::Options CLIOpts("gross", "GRaph Optimizer sOley for cS241");
  InitializeCLIOptions(CLIOpts);
  auto GrossOpts = CLIOpts.parse(argc, argv);

  if(GrossOpts.count("help")) {
    std::cout << CLIOpts.help() << "\n";
    return 0;
  } else if(GrossOpts.count("version")) {
    std::cout << "v0.94.87\n";
    return 0;
  }

  return 0;
}
