
# Meta build system for unity build and builds with shared sources between different executables.
#
# Add executables using add_executable_shared(EXE SRC LIBS EXTRA).
#
# Build the executables using either separate_build(), shared_build() or unity_build().
#

# create a unity build file for the executable UB_SUFFIX with the sources stored in the variable named by SOURCE_VARIABLE_NAME
# the name of the resulting unity build file will be stored in the variable named by SOURCE_VARIABLE_NAME
function(enable_unity_build UB_SUFFIX SOURCE_VARIABLE_NAME)
	set(files ${${SOURCE_VARIABLE_NAME}})
	
	# Sort the file list to ensure we get the same files order on all machines
	list(SORT files)
	
	# Generate a unique filename for the unity build translation unit
	set(unit_build_file ${CMAKE_CURRENT_BINARY_DIR}/ub_${UB_SUFFIX}.cpp)
	
	# Exclude all translation units from compilation
	set_source_files_properties(${files} PROPERTIES HEADER_FILE_ONLY true)
	
	# Open the ub file  
	file(WRITE ${unit_build_file} "// Unity build generated by CMake\n\n")
	
	# Add include statement for each translation unit
	list(LENGTH files numfiles)
	set(currentIdx 1)
	foreach(source_file IN LISTS files)
		
		get_filename_component(source_file "${source_file}" ABSOLUTE)
		
		string(REGEX REPLACE ".*\\/" "" short_file "${source_file}")
		file(APPEND ${unit_build_file} "#pragma message (\"[${currentIdx}/${numfiles}] Compiling ${short_file}...\")\n")
		file(APPEND ${unit_build_file} "#include \"${source_file}\"\n\n")
		math(EXPR currentIdx "${currentIdx} + 1")
		
	endforeach(source_file)
	
	# Complement list of translation units with the name of ub
	set(${SOURCE_VARIABLE_NAME} ${${SOURCE_VARIABLE_NAME}} ${unit_build_file} PARENT_SCOPE)
	
	# Put ub file at the root of the project
	source_group( "" FILES ${unit_build_file} )
endfunction(enable_unity_build)


unset(SHARED_BUILD_EXECUTABLES CACHE)

# Add an executable to be build by either separate_build(), shared_build() or unity_build()
#  EXE   Name of the executable to add.
#  TYPE	 Type of exe (ex: WIN32)
#  SRC   The executables source files.
#  LIBS  Libraries to link the executable against.
#  EXTRA Additional arguments to pass to add_executable() but not shared amongs executables or included in unity builds.
function(add_executable_shared EXE TYPE SRC LIBS EXTRA)
	list(REMOVE_DUPLICATES SRC)
	list(REMOVE_DUPLICATES LIBS)
	set(SHARED_BUILD_${EXE}_TYPE "${TYPE}" CACHE INTERNAL "")
	set(SHARED_BUILD_${EXE}_SOURCES "${SRC}" CACHE INTERNAL "")
	set(SHARED_BUILD_${EXE}_LIBS "${LIBS}" CACHE INTERNAL "")
	set(SHARED_BUILD_${EXE}_EXTRA "${EXTRA}" CACHE INTERNAL "")
	set(SHARED_BUILD_EXECUTABLES ${SHARED_BUILD_EXECUTABLES} ${EXE} CACHE INTERNAL "")
endfunction(add_executable_shared)


# Calculate the intersection of the lists SRC1 and SRC2 and store the result in the variable named by DEST.
function(intersect DEST SRC1 SRC2)
	
	set(dest "")
	
	foreach(src IN LISTS SRC1)
		list(FIND SRC2 "${src}" found)
		if(NOT found EQUAL -1)
			list(APPEND dest "${src}")
		endif()
	endforeach()
	
	set(${DEST} "${dest}" PARENT_SCOPE)
	
endfunction(intersect)


function(_shared_build_helper LIB LIST EXESET FIRST)
	
	set(list ${LIST})
	set(first ${FIRST})
	
	# Find common sources and extract static libraries.
	foreach(exe IN LISTS LIST)
		
		list(REMOVE_ITEM list ${exe})
		
		set(exeset ${EXESET} ${exe})
		
		_shared_build_helper(${LIB}_${exe} "${list}" "${exeset}" ${first})
		
		set(first 0)
		
	endforeach(exe)
	
	# Find sources common to all executables in the current set.
	set(first 1)
	foreach(exe IN LISTS EXESET)
		if(first)
			set(first 0)
			set(common_src "${SHARED_BUILD_${exe}_SOURCES}")
		else()
			intersect(common_src "${common_src}" "${SHARED_BUILD_${exe}_SOURCES}")
		endif()
	endforeach(exe)
	
	# We found common sources!
	if(NOT "${common_src}" STREQUAL "")
		
		list(LENGTH LIST all_execs)
		if(FIRST AND all_execs EQUAL 0)
			set(lib common)
		else()
			set(lib _${LIB}_common)
		endif()
		
		# Add a new library for the common sources
		add_library(${lib} STATIC ${common_src})
		
		# Remove sources from executables and link the library instead.
		foreach(exe IN LISTS EXESET)
			
			set(uncommon_src "${SHARED_BUILD_${exe}_SOURCES}")
			foreach(src IN LISTS common_src)
				list(REMOVE_ITEM uncommon_src ${src})
			endforeach(src)
			set(SHARED_BUILD_${exe}_SOURCES "${uncommon_src}" CACHE INTERNAL "")
			
			set(SHARED_BUILD_${exe}_LIBS ${lib} "${SHARED_BUILD_${exe}_LIBS}" CACHE INTERNAL "")
			
		endforeach(exe)
		
	endif()
	
endfunction(_shared_build_helper)


function(_shared_build_cleanup)
	
	foreach(exe IN LISTS SHARED_BUILD_EXECUTABLES)
		unset(SHARED_BUILD_${exe}_TYPE CACHE)
		unset(SHARED_BUILD_${exe}_SOURCES CACHE)
		unset(SHARED_BUILD_${exe}_LIBS CACHE)
		unset(SHARED_BUILD_${exe}_EXTRA CACHE)
	endforeach(exe)
	
	unset(SHARED_BUILD_EXECUTABLES CACHE)
	
endfunction(_shared_build_cleanup)


# Build each executable separately.
function(separate_build)
	
	foreach(exe IN LISTS SHARED_BUILD_EXECUTABLES)
		add_executable(${exe} ${SHARED_BUILD_${exe}_TYPE} ${SHARED_BUILD_${exe}_SOURCES} ${SHARED_BUILD_${exe}_EXTRA})
		target_link_libraries(${exe} ${SHARED_BUILD_${exe}_LIBS})
		install(TARGETS ${exe} RUNTIME DESTINATION bin)
	endforeach(exe)
	
	_shared_build_cleanup()
	
endfunction(separate_build)


# Build each source file separately and extract common source files into static libraries.
function(shared_build)
	
	set(list1 ${SHARED_BUILD_EXECUTABLES})
	
	set(first 1)
	
	# Find common sources and extract static libraries.
	foreach(exe1 IN LISTS SHARED_BUILD_EXECUTABLES)
		list(REMOVE_ITEM list1 ${exe1})
		set(list2 ${list1})
		# Require two source sets before calling _shared_build_helper so we don't create static libraries for individual executables!
		foreach(exe2 IN LISTS list1)
			list(REMOVE_ITEM list2 ${exe2})
			set(exeset ${exe1} ${exe2})
			_shared_build_helper(${exe1}_${exe2} "${list2}" "${exeset}" ${first})
			set(first 0)
		endforeach(exe2)
	endforeach(exe1)
	
	separate_build()
	
endfunction(shared_build)


# Build each executable by including all the source files into one big master file.
function(unity_build)
	
	foreach(exe IN LISTS SHARED_BUILD_EXECUTABLES)
		enable_unity_build(${exe} SHARED_BUILD_${exe}_SOURCES)
		add_executable(${exe} ${SHARED_BUILD_${exe}_TYPE} ${SHARED_BUILD_${exe}_SOURCES} ${SHARED_BUILD_${exe}_EXTRA})
		target_link_libraries(${exe} ${SHARED_BUILD_${exe}_LIBS})
		install(TARGETS ${exe} RUNTIME DESTINATION bin)
	endforeach(exe)
	
	_shared_build_cleanup()
	
endfunction(unity_build)
