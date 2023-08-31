import os
from conans import ConanFile
from conan.tools.cmake import CMake
from conan.tools.files import rename


class AsyncNatsCppConan(ConanFile):
    name = "async_nats"
    version = "0.1.0"
    license = "MIT"
    author = "Artem Vasiliev conan@yazasnyal.dev"
    url = "https://github.com/YaZasnyal/async_nats.cpp"
    description = "Binding to the async_nats.rs library"
    topics = ("asio", "nats", "messaging")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "CMakeToolchain"
    exports_sources = "../*"
    no_copy_source = True

    def requirements(self):
        self.requires("boost/1.79.0")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include", src="include")
        self.copy("*.hpp", dst="include", src="include")

        if self.options.shared:
            self.copy("nats_fabric.dll", dst="bin", keep_path=False)
            self.copy("nats_fabric.dll.lib", dst="lib", keep_path=False)
            rename(
                self,
                os.path.join(self.package_folder, "lib", "nats_fabric.dll.lib"),
                os.path.join(self.package_folder, "lib", "nats_fabric.lib"),
            )
            self.copy("libnats_fabric.so", dst="lib", keep_path=False)
            self.copy("nats_fabric.dylib*", dst="lib", keep_path=False)
        else:
            self.copy("nats_fabric.lib", dst="lib", keep_path=False)
            self.copy("libnats_fabric.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["nats_fabric"]
        self.cpp_info.requires = ["boost::headers"]
        if self.settings.os in ["Linux", "FreeBSD"]:
            pass
        elif self.settings.os == "Windows":
            self.cpp_info.system_libs = [
                "ntdll",
                "Crypt32",
                "Ncrypt",
                "Secur32",
                "ws2_32",
            ]
        elif self.settings.os == "Macos":
            self.cpp_info.frameworks.append("CoreFoundation")
            self.cpp_info.frameworks.append("Security")
