require File.dirname(__FILE__) + '/cmake.rb'
require File.dirname(__FILE__) + '/git.rb'

module Mmdai

class Alsoft < Thor
  include Build::CMake
  include VCS::Git

  desc "build", "build OpenAL Soft"
  method_options :flag => :boolean
  def build
    checkout
    invoke_build
  end

  desc "clean", "clean built OpenAL soft libraries"
  def clean
    invoke_clean
  end

protected
  def get_uri
    "git://repo.or.cz/openal-soft.git"
  end

  def get_directory_name
    "openal-soft-src"
  end

  def get_tag_name
    "openal-soft-1.15.1"
  end

  def get_build_options(build_type, extra_options)
    return {
      :dlopen => false,
      :utils => false,
      :examples => false,
      :alsoft_config => false
    }
  end

end

end

