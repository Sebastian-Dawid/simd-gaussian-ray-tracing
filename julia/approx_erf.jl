using Plots, SpecialFunctions, LaTeXStrings

function cubic_spline_interpolation(x, y)
    h = [x[i] - x[i-1] for i in eachindex(x)[2:end]]
    b = [ 3 * ((y[i+1] - y[i])/h[i] - (y[i] - y[i-1])/h[i-1]) for i in eachindex(y)[2:end-1]]
    a_0 = y[2:end]
    A = zeros(length(h) - 1, length(h) - 1)
    for i in eachindex(h)[1:end-1]
        if i-1 > 0
            A[i, i-1] = h[i]
        end
        A[i, i] = 2*(h[i] + h[i+1])
        if i+1 <= length(h)-1
            A[i, i+1] = h[i+1]
        end
    end
    Ai = inv(A)
    a_2 = zeros(length(h) + 1)
    a_2[2:end-1] = Ai * b
    a_3 = [(a_2[i + 1] - a_2[i])/(3 * h[i]) for i in eachindex(h)]
    a_1 = [(y[i+1] - y[i])/h[i] + a_2[i+1]*h[i] - a_3[i]*h[i]^2 for i in eachindex(h)]
    p = s -> a_0 + a_1 .* (s .- x[2:end]) + a_2[2:end] .* (s .- x[2:end]).^2 + a_3 .* (s .- x[2:end]).^3
    function get_index(t)
        t = t < x[1] ? x[1] : t
        t = t > x[end] ? x[end] : t
        return argmax([x[i] <= t <= x[i+1] for i in eachindex(x)[1:end-1]])
    end
    return (t -> p(t)[get_index(t)]), [a_0, a_1, a_2, a_3]
end

function generate_c(f=erf, name="erf", x=-6.0:.5:6.0, lower=-1.0, upper=1.0)
    _, a = cubic_spline_interpolation(x, f.(x))
    factors = map(x -> string.(Float32.(x)) .* "f", Vector{Float32}.(a))
    xs = "(x - " .* string.(x[2:end]) .* "f)"
    polynomials = "((" .* factors[4] .* " * " .* xs .* " + " .* factors[3][2:end] .* ") * " .* xs
    polynomials = polynomials .* " + " .* factors[2] .* ") * " .* xs .* " + " .* factors[1]
    returns = "return " .* polynomials .* ";"
    fun = "/// generated using spline interpolation with supports $(x)
f32 spline_"*name*"(const f32 x)
{
    if (x <= $(Float32(x[2]))f) return $(lower)f;\n"
    for i in eachindex(returns)[1:end-1]
        fun = fun .* "    else if (x < $(x[i+2])f) { " .* returns[i] .* " }\n"
    end
    fun = fun .* "    return $(upper)f;
}"
    println(fun)
end

function generate_simd_c(f=erf, name="erf", x=-6.0:.5:6.0, lower=-1.0, upper=1.0)
    _, a = cubic_spline_interpolation(x, f.(x))
    factors = map(x -> "simd::set1(" .* string.(x) .* "f)", Vector{Float32}.(a))
    xs = "(x - " .* "simd::set1(" .* string.(x[2:end]) .* "f))"
    polynomials = "((" .* factors[4] .* " * " .* xs .* " + " .* factors[3][2:end] .* ") * " .* xs
    polynomials = polynomials .* " + " .* factors[2] .* ") * " .* xs .* " + " .* factors[1]
    fun = "/// generated using spline interpolation with supports $(x)
simd::Vec<simd::Float> simd_spline_"*name*"(const simd::Vec<simd::Float> x)
{
    simd::Vec<simd::Float> value = simd::set1($(upper));
    simd::ifelse(x <= simd::set1($(Float32(x[2]))f), simd::set1($(lower)), value);\n"
    for i in eachindex(polynomials)[1:end-1]
        fun = fun .* "    value = simd::ifelse(simd::bit_and(simd::set1($(x[i+1])f) <= x, x <= simd::set1($(x[i+2])f)), " .* polynomials[i] .* ", value);\n"
    end
    fun = fun .* "    return value;
}"
    println(fun)
end

function cmp_erf(;start=-6, step=.1, finish=6, spline_step=.5, taylor_degree=25)
    default(titlefont=(10), legendfontsize=8)
    spline_erf, _ = cubic_spline_interpolation(start:spline_step:finish, erf.(start:spline_step:finish))
    taylor_erf = z -> 2/sqrt(π) * sum([ z/(2*n + 1) * prod([ (-z^2)/k for k in 1:n]) for n in 0:taylor_degree ])
    spline_err = x -> abs(spline_erf(x) - erf(x))
    taylor_err = x -> abs(taylor_erf(x) - erf(x))
    spline_plt = plot(start:step:finish, spline_erf, label="approx", title="Spline interpolation, $(length(start:spline_step:finish)) samples", dpi=300)
    plot!(start:step:finish, spline_err, label="err")
    taylor_plt = plot(start:step:finish, taylor_erf, ylims=(-1.1, 1.1), label="approx", title="Taylor series degree=$(taylor_degree)", dpi=300)
    plot!(start:step:finish, taylor_err, label="err")
    return spline_plt, taylor_plt
end

if abspath(PROGRAM_FILE) == @__FILE__
    spline_plt, taylor_plt = cmp_erf()
    savefig(spline_plt, "../images/spline_erf.png")
    savefig(taylor_plt, "../images/taylor_erf.png")
end
