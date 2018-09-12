add_library(proto_stateless_validation
    transaction_proto_validator.cpp
    )

target_link_libraries(proto_stateless_validation
    schema
    )
