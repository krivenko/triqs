from wrap_generator import *

# The many_body_operators module
module = module_(full_name = "pytriqs.operators.operators", doc = "Doc to be written")
module.add_include("<triqs/operators/many_body_operator.hpp>")
module.add_include("<triqs/python_tools/converters/pair.hpp>")
module.add_include("<triqs/python_tools/converters/vector.hpp>")
module.add_include("<triqs/python_tools/converters/variant_int_string.hpp>")
module.add_using("namespace triqs::utility")


# The operator class
op = class_(
        py_type = "Operator",
        c_type = "many_body_operator<double>",
        c_type_absolute = "triqs::utility::many_body_operator<double>",
        is_printable= True,
        arithmetic = ("algebra","with_unit","with_unary_minus","double")
        )

op.add_constructor(signature="()", doc="create zero operator")
op.add_constructor(signature="(double x)", doc="create a constant operator")
op.add_method("bool is_zero()", doc = "Boolean : is the operator null ?")
op.add_iterator(c_cast_type="std::pair<std::vector<std::pair<bool,many_body_operator<double>::indices_t>>,double>")

module.add_class(op)

# Add various overload of c, c_dag to the module Annihilation & Creation operators
for name, doc in [("c","annihilation operator"), ("c_dag","creation operator"), ("n","number operator")] :
    for sign in [
            "",
            "std::string ind1",
            "std::string ind1, std::string ind2",
            "int i, std::string ind1",
            "std::string ind1, int i",
            "int i, int j"
            ]:
        module.add_function(name = name, signature="many_body_operator<double>(%s)"%sign, doc=doc)

module.add_function("many_body_operator<double> dagger(many_body_operator<double> Op)", doc= "Return the dagger of the operator")

module.generate_code()

