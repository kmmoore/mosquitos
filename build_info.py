#! /usr/bin/env python

from subprocess import Popen, PIPE
import datetime

git_info = Popen(["git", "describe", "--dirty", "--always"], stdout=PIPE).communicate()[0]
git_info = git_info.strip()

print "#include \"build_info.h\""
print "const char build_git_info[] = \"{git_info}\";".format(git_info=git_info)
print "const char build_time[] = \"{time}\";".format(time=datetime.datetime.now())