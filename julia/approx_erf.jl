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

function generate_c(x=-6:.5:6)
    _, a = cubic_spline_interpolation(x, erf.(x))
    factors = map(x -> string.(x) .* "f", Vector{Float32}.(a))
    xs = "(x - " .* string.(x[2:end]) .* "f)"
    xs2 = xs .* " * " .* xs
    xs3 = xs2 .* " * " .* xs
    p3 = factors[4] .* " * " .* xs3
    p2 = factors[3][2:end] .* " * " .* xs2
    p1 = factors[2] .* " * " .* xs
    polynomials = p3 .* " + " .* p2 .* " + " .* p1 .* " + " .* factors[1]
    returns = "return " .* polynomials .* ";"
    fun = "float spline_erf(float x)
{
    if (x <= $(x[2])f) return -1.f;\n"
    for i in eachindex(returns)[1:end-1]
fun = fun .* "    else if ($(x[i+1])f <= x && x <= $(x[i+2])f) { " .* returns[i] .* " }\n"
    end
    fun = fun .* "    return 1.f;
}"
    println(fun)
end

function main(;start=-6, step=.1, finish=6, spline_step=.5, taylor_degree=25)
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
    spline_plt, taylor_plt = main()
    savefig(spline_plt, "../images/spline_erf.png")
    savefig(taylor_plt, "../images/taylor_erf.png")
end
