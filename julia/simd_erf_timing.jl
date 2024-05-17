using CSV, DataFrames, Plots, LaTeXStrings

function load_data()
    data_path = "./"*dirname(relpath(@__FILE__))*"/"
    data = CSV.File(data_path*"../csv/simd_erf.csv") |> DataFrame
    count = Vector(data[:, 1])
    t = Vector(data[:, 2])
    plot(count, t, dpi=300, label="simd", xlabel=L"$n$", ylabel=L"$ns$", title=L"Time to calculate $n$ values of $erf$.")
    t = Vector(data[:, 3])
    plot!(count, t, label="simd mirror")
    t = Vector(data[:, 4])
    plot!(count, t, label="spline")
    t = Vector(data[:, 5])
    plot!(count, t, label="std::erf")
end

if abspath(PROGRAM_FILE) == @__FILE__
    img_dir = "./"*dirname(relpath(@__FILE__))*"/../images"
    if !isdir(img_dir)
        mkdir(img_dir)
    end
    savefig(load_data(), img_dir*"/simd_erf_timing.png")
end

