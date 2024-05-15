using CSV, DataFrames, Plots, LaTeXStrings

function load_data()
    data_path = "./"*dirname(relpath(@__FILE__))*"/"
    data = CSV.File(data_path*"../csv/erf.csv") |> DataFrame
    x = Vector(data[:, 1])
    y_spline = Vector(data[:, 2])
    approx = plot(x, y_spline, dpi=300, label="spline", xlabel=L"x", ylabel=L"$y$", title="Approximations")
    y_mirror = Vector(data[:, 3])
    plot!(x, y_mirror, label="spline mirror")
    y_taylor = Vector(data[:, 4])
    plot!(x, y_taylor, label="taylor")
    y = Vector(data[:, 5])
    plot!(x, y, label="std::erf")
    err = plot(x, abs.(y - y_spline), dpi=300, label="spline", xlabel=L"x", ylabel="err", title="Error")
    plot!(x, abs.(y - y_mirror), label="mirror")
    plot!(x, abs.(y - y_taylor), label="taylor")
    return approx, err
end

if abspath(PROGRAM_FILE) == @__FILE__
    img_dir = "./"*dirname(relpath(@__FILE__))*"/../images"
    if !isdir(img_dir)
        mkdir(img_dir)
    end
    approx, err = load_data()
    savefig(plot(approx, err), img_dir*"/cmp_erf.png")
end
