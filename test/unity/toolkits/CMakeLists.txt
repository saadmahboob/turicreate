project(unity_test_toolkits)

add_subdirectory(feature_engineering)
add_subdirectory(ml_data_2)
add_subdirectory(synthetic_timings)
add_subdirectory(recsys)
add_subdirectory(sparse_similarity)
add_subdirectory(pattern_mining)
add_subdirectory(neural_net)
add_subdirectory(object_detection)
add_subdirectory(coreml_export)

make_boost_test(kmeans_test.cxx
  REQUIRES unity_clustering util eigen
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )

make_boost_test (nearest_neighbors.cxx
  REQUIRES unity_nearest_neighbors eigen
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )

make_boost_test (topic_model.cxx
  REQUIRES unity_text
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )

make_boost_test (test_evaluation.cxx
  REQUIRES unity_evaluation unity_recsys eigen
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )
