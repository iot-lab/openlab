#
# This file is part of HiKoB Openlab.
#
# HiKoB Openlab is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation, version 3.
#
# HiKoB Openlab is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with HiKoB Openlab. If not, see
# <http://www.gnu.org/licenses/>.
#
# Copyright (C) 2011 HiKoB.
#

# Add the test directory
add_subdirectory(tests)

# Add the example directory
add_subdirectory(examples)

# Add the nancy directory
add_subdirectory(newt_examples)

# IoT-LAB specific directories
add_subdirectory(iotlab)
add_subdirectory(iotlab_examples)
add_subdirectory(iotlab_tests)

# Sensbio specific directories
add_subdirectory(sensbio)

# Education Practical Works examples
add_subdirectory(education)

# Bachelor project
add_subdirectory(bachelor)