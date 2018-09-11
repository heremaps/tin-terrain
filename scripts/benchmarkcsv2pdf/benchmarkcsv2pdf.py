#!/usr/bin/env python3

import sys
import argparse
import csv
import math

import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt

from matplotlib.backends.backend_pdf import PdfPages

#returns a list of dicts with the fields from the header row
def read_csv_with_header_to_dicts(input_file):
    out = []
    
    with open(input_file, "rt") as csvfile:
        csvreader = csv.reader(csvfile, delimiter=",", quotechar="\'")
        
        header_row = next(csvreader)
        column_to_name = {}
        for i in range(0, len(header_row)):
            column_name = header_row[i]
            #strip leading hashtags
            while column_name.startswith("#"):
                column_name = column_name[1:]
            column_to_name[i] = column_name
        
        for data_row in csvreader:
            data_dict = {}
            #skip empty rows
            if len(data_row) == 0:
                continue
            #skip rows that start with # - makes concatenating .csv files simple
            if data_row[0].startswith("#"):
                continue
            for i in range(0, len(data_row)):
                column_name = column_to_name[i]
                cell_value = data_row[i]
                data_dict[column_name] = cell_value
            out.append(data_dict)
            
    return out


def list_methods(benchmark_data):
    methods = set()
    for row in benchmark_data:
        methods.add(row["method_name"])
    return list(methods)

def list_input_files(benchmark_data):
    input_files = set()
    for row in benchmark_data:
        input_files.add(row["input_file"])
    return list(input_files)


def get_input_file_description(benchmark_data):
    row = benchmark_data[0]
    #input_num_points,input_width,input_height
    return '%s\n%sx%spx / %s points' % (row["input_file"], row["input_width"], row["input_height"], row["input_num_points"])

def apply_suptitle(fig, benchmark_data, method_name=""):
    supt = get_input_file_description(benchmark_data)
    if len(method_name) != 0:
        supt = 'method: %s\n%s' % (method_name, supt)
    fig.suptitle(supt, fontsize=8)

def new_figure():
    return plt.subplots(figsize=(20, 15))
    
def filtered_data(benchmark_data, field, value):
    return [ row for row in benchmark_data if row[field] == value ]

def sort_points(points):
    points.sort(key=lambda tup: tup[0], reverse=True)
    
def points_to_xy(points):
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    return (xs, ys)

def graph_error_over_faces(pp, benchmark_data):
    fig, ax = new_figure()
    
    methods = list_methods(benchmark_data)
    for method in methods:
        points = []
        for row in benchmark_data:
            if row["method_name"] == method:
                num_faces = int(row["num_faces"])
                error = get_rms_error(row)
                points.append((num_faces, error))

        sort_points(points)
        xs,ys = points_to_xy(points)    
        ax.plot(xs, ys, label=method)

    ax.set_ylabel("RMS error")
    ax.set_xlabel("#faces")
    ax.legend()
    
    apply_suptitle(fig, benchmark_data)
    #plt.show()
    pp.savefig()

def graph_faces_over_error(pp, benchmark_data):
    fig, ax = new_figure()
    
    methods = list_methods(benchmark_data)
    for method in methods:
        points = []
        for row in benchmark_data:
            if row["method_name"] == method:
                num_faces = int(row["num_faces"])
                error = get_rms_error(row)
                points.append((error, num_faces))

        sort_points(points)
        xs,ys = points_to_xy(points)    
        ax.plot(xs, ys, label=method)

    ax.set_ylabel("#faces")
    ax.set_xlabel("RMS error")
    ax.legend()
    
    apply_suptitle(fig, benchmark_data)
    #plt.show()
    pp.savefig()
    


def graph_time_over_faces(pp, benchmark_data):
    fig, ax = new_figure()
    
    methods = list_methods(benchmark_data)
    for method in methods:
        points = []
        for row in benchmark_data:
            if row["method_name"] == method:
                num_faces = int(row["num_faces"])
                time = float(row["meshing_time_seconds"])
                points.append((num_faces, time))

        sort_points(points)
        xs,ys = points_to_xy(points)    
        ax.plot(xs, ys, label=method)

    ax.set_ylabel("time (s)")
    ax.set_xlabel("#faces")
    ax.legend()
    
    apply_suptitle(fig, benchmark_data)
    #plt.show()
    pp.savefig()

def get_rms_error(row):
    mean_error = float(row["mean_error"])
    std_dev_error = float(row["std_dev_error"])
    rms = math.sqrt(mean_error*mean_error + std_dev_error*std_dev_error)
    return rms


def polygon_usage(num_faces, rms_error, raster_points):
    return (num_faces * rms_error) / raster_points
    
def polygon_efficiency(num_faces, rms_error, raster_points):
    inv_efficiency = (num_faces/raster_points) + polygon_usage(num_faces, rms_error, raster_points)
    return 1.0/inv_efficiency

def graph_polygon_efficiency_over_meshing_time(pp, benchmark_data):
    fig, ax = new_figure()
    
    methods = list_methods(benchmark_data)
    for method in methods:
        
        points = []
        for row in benchmark_data:
            if row["method_name"] == method:
                num_faces = int(row["num_faces"])
                rms_error = get_rms_error(row)
                raster_points = int(row["input_num_points"])
                meshing_time_seconds = float(row["meshing_time_seconds"])
                points.append((meshing_time_seconds, polygon_efficiency(num_faces, rms_error, raster_points)))
        
        sort_points(points)
        xs,ys = points_to_xy(points)
        ax.plot(xs, ys, label=method)

    ax.set_ylabel("polygon efficiency (higher is better)")
    ax.set_xlabel("meshing time (s)")
    ax.legend()
    
    apply_suptitle(fig, benchmark_data)
    #plt.show()
    pp.savefig()
    
def graph_polygon_efficiency_over_faces(pp, benchmark_data):
    fig, ax = new_figure()
    
    #ax.set_xlim(0, 20000000)
    #ax.set_ylim(0, 40)
    
    methods = list_methods(benchmark_data)
    for method in methods:
        
        points = []
        for row in benchmark_data:
            if row["method_name"] == method:
                num_faces = int(row["num_faces"])
                rms_error = get_rms_error(row)
                raster_points = int(row["input_num_points"])
                points.append((num_faces, polygon_efficiency(num_faces, rms_error, raster_points)))
        
        sort_points(points)
        xs,ys = points_to_xy(points)
        ax.plot(xs, ys, label=method)

    ax.set_ylabel("polygon efficiency (higher is better)")
    ax.set_xlabel("#faces")
    ax.legend()
    
    apply_suptitle(fig, benchmark_data)
    #plt.show()
    pp.savefig()
    

def graph_polygon_usage_over_meshing_time(pp, benchmark_data):
    fig, ax = new_figure()
    
    methods = list_methods(benchmark_data)
    for method in methods:        
        points = []
        for row in benchmark_data:
            if row["method_name"] == method:
                num_faces = int(row["num_faces"])
                rms_error = get_rms_error(row)
                raster_points = int(row["input_num_points"])
                meshing_time_seconds = float(row["meshing_time_seconds"])
                points.append((meshing_time_seconds, polygon_usage(num_faces, rms_error, raster_points)))
        
        sort_points(points)
        xs,ys = points_to_xy(points)
        ax.plot(xs, ys, label=method)

    ax.set_ylabel("polygon usage (lower is better)")
    ax.set_xlabel("meshing time (s)")
    ax.legend()
    
    apply_suptitle(fig, benchmark_data)
    #plt.show()
    pp.savefig()

def graph_polygon_usage_over_faces(pp, benchmark_data):
    fig, ax = new_figure()
    
    methods = list_methods(benchmark_data)
    for method in methods:
        #if method == "regular":
        #    continue
        points = []
        for row in benchmark_data:
            if row["method_name"] == method:
                num_faces = int(row["num_faces"])
                rms_error = get_rms_error(row)
                raster_points = int(row["input_num_points"])
                points.append((num_faces, polygon_usage(num_faces, rms_error, raster_points)))
        
        sort_points(points)
        xs,ys = points_to_xy(points)
        ax.plot(xs, ys, label=method)

    ax.set_ylabel("polygon usage (lower is better)")
    ax.set_xlabel("#faces")
    ax.legend()
    
    apply_suptitle(fig, benchmark_data)
    #plt.show()
    pp.savefig()

def graph_combined_num_faces_error_over_param_max_error_terra_vs_zemlya(pp, benchmark_data):
    
    terra_faces_points = []
    terra_error_points = []
    
    zemlya_faces_points = []
    zemlya_error_points = []
    
    
    for row in benchmark_data:
        num_faces = int(row["num_faces"])
        param_max_error = float(row["param_max_error"])
        error = get_rms_error(row)
        
        if row["method_name"] == "terra":
            terra_faces_points.append((param_max_error, num_faces))
            terra_error_points.append((param_max_error, error))
            
        if row["method_name"] == "zemlya":
            zemlya_faces_points.append((param_max_error, num_faces))
            zemlya_error_points.append((param_max_error, error))
            
    sort_points(terra_faces_points)
    sort_points(terra_error_points)
    sort_points(zemlya_faces_points)
    sort_points(zemlya_error_points)

    fig, ax1 = new_figure()
    
    xs,ys = points_to_xy(terra_faces_points)
    ax1.plot(xs, ys, marker=".", linestyle="solid", color="C0", label="terra #faces")

    xs,ys = points_to_xy(zemlya_faces_points)
    ax1.plot(xs, ys, marker=".", linestyle="solid", color="C1", label="zemlya #faces")
    
    ax1.set_xlabel("param_max_error")
    ax1.set_ylabel("#faces")
    ax1.set_xticks(xs)
    
    
    ax2 = ax1.twinx()
    ax2.set_ylabel("RMS error")
    
    xs,ys = points_to_xy(terra_error_points)
    ax2.plot(xs, ys, marker=".", linestyle="dashed", color="C0", label="terra RMS error")
    
    xs,ys = points_to_xy(zemlya_error_points)
    ax2.plot(xs, ys, marker=".", linestyle="dashed", color="C1", label="zemlya RMS error")
    
    fig.legend()
    
    apply_suptitle(fig, benchmark_data)
    #plt.show()
    pp.savefig()


def graph_combined_num_faces_time_over_param_max_error_terra_vs_zemlya(pp, benchmark_data):
    terra_faces_points = []
    terra_time_points = []
    
    zemlya_faces_points = []
    zemlya_time_points = []
    
    
    for row in benchmark_data:
        num_faces = int(row["num_faces"])
        param_max_error = float(row["param_max_error"])
        time = float(row["meshing_time_seconds"])
        
        if row["method_name"] == "terra":
            terra_faces_points.append((param_max_error, num_faces))
            terra_time_points.append((param_max_error, time))
            
        if row["method_name"] == "zemlya":
            zemlya_faces_points.append((param_max_error, num_faces))
            zemlya_time_points.append((param_max_error, time))
            
    fig, ax1 = new_figure()

    sort_points(terra_faces_points)
    sort_points(terra_time_points)
    sort_points(zemlya_faces_points)
    sort_points(zemlya_time_points)

    xs,ys = points_to_xy(terra_faces_points)
    ax1.plot(xs, ys, marker=".", linestyle="solid", color="C0", label="terra #faces")

    xs,ys = points_to_xy(zemlya_faces_points)
    ax1.plot(xs, ys, marker=".", linestyle="solid", color="C1", label="zemlya #faces")
    
    ax1.set_xlabel("param_max_error")
    ax1.set_ylabel("#faces")
    ax1.set_xticks(xs)

    
    ax2 = ax1.twinx()
    ax2.set_ylabel("time (s)")
    
    xs,ys = points_to_xy(terra_time_points)
    ax2.plot(xs, ys, marker=".", linestyle="dashed", color="C0", label="terra meshing time")
    
    xs,ys = points_to_xy(zemlya_time_points)
    ax2.plot(xs, ys, marker=".", linestyle="dashed", color="C1", label="zemlya meshing time")
    
    fig.legend()
    
    apply_suptitle(fig, benchmark_data)
    #plt.show()
    pp.savefig()


def graph_polygon_efficiency_over_param_max_error_terra_vs_zemlya(pp, benchmark_data):
    fig, ax = new_figure()
    
    terra_points = []
    zemlya_points = []
    
    for row in benchmark_data:
        num_faces = int(row["num_faces"])
        rms_error = get_rms_error(row)
        raster_points = int(row["input_num_points"])
        param_max_error = float(row["param_max_error"])
        efficiency = polygon_efficiency(num_faces, rms_error, raster_points)
        
        if row["method_name"] == "terra":
            terra_points.append((param_max_error, efficiency))
        if row["method_name"] == "zemlya":
            zemlya_points.append((param_max_error, efficiency))
            
    
    sort_points(terra_points)
    sort_points(zemlya_points)
    
    xs,ys = points_to_xy(terra_points)
    ax.plot(xs, ys, label="terra")

    xs,ys = points_to_xy(zemlya_points)
    ax.plot(xs, ys, label="zemlya")

    ax.set_ylabel("polygon efficiency (higher is better)")
    ax.set_xlabel("param_max_error")
    ax.set_xticks(xs)
    ax.legend()
    
    apply_suptitle(fig, benchmark_data)
    #plt.show()
    pp.savefig()
    
    
def graph_combined_faces_error_over_floatparam(pp, benchmark_data, method_name, param_name):
    num_faces_points = []
    error_points = []
    
    for row in benchmark_data:
        if row["method_name"] == method_name:
            num_faces = int(row["num_faces"])
            error = get_rms_error(row)
            param = float(row[param_name])
            num_faces_points.append((param, num_faces))    
            error_points.append((param, error))

    fig, ax1 = new_figure()

    sort_points(num_faces_points)    
    sort_points(error_points)
    
    xs,ys = points_to_xy(num_faces_points)
    ax1.plot(xs, ys, marker=".", color="C0", label="#faces")
    ax1.set_xticks(xs)
    ax1.set_xlabel(param_name)
    ax1.set_ylabel("#faces")
    
    ax2 = ax1.twinx()
    xs,ys = points_to_xy(error_points)
    ax2.plot(xs, ys, marker=".", color="C1", label="RMS error")
    
        
    apply_suptitle(fig, benchmark_data, method_name)
    fig.legend()
    
    #plt.show()
    pp.savefig()
        
def graph_combined_faces_time_over_floatparam(pp, benchmark_data, method_name, param_name):
    num_faces_points = []
    time_points = []
    
    for row in benchmark_data:
        if row["method_name"] == method_name:
            num_faces = int(row["num_faces"])
            time = float(row["meshing_time_seconds"])
            param = float(row[param_name])
            num_faces_points.append((param, num_faces))    
            time_points.append((param, time))

    fig, ax1 = new_figure()

    sort_points(num_faces_points)    
    sort_points(time_points)
    
    xs,ys = points_to_xy(num_faces_points)
    ax1.plot(xs, ys, marker=".", color="C0", label="#faces")
    ax1.set_xticks(xs)
    ax1.set_xlabel(param_name)
    ax1.set_ylabel("#faces")
    
    ax2 = ax1.twinx()
    xs,ys = points_to_xy(time_points)
    ax2.plot(xs, ys, marker=".", color="C1", label="meshing time")
    ax2.set_ylabel("meshing time (s)")
    
        
    apply_suptitle(fig, benchmark_data, method_name)
    fig.legend()
    
    #plt.show()
    pp.savefig()
    

def benchmarkscsv2pdf(input_file, output_file):
    benchmark_data = read_csv_with_header_to_dicts(input_file)

    with PdfPages(output_file) as pp:
        input_files = list_input_files(benchmark_data)
        for input_file in input_files:
            input_file_benchmark_data = filtered_data(benchmark_data, "input_file", input_file)
            
            graph_error_over_faces(pp, input_file_benchmark_data)
            graph_faces_over_error(pp, input_file_benchmark_data)
            graph_time_over_faces(pp, input_file_benchmark_data)
            graph_polygon_usage_over_meshing_time(pp, input_file_benchmark_data)
            graph_polygon_usage_over_faces(pp, input_file_benchmark_data)            
            graph_polygon_efficiency_over_meshing_time(pp, input_file_benchmark_data)
            graph_polygon_efficiency_over_faces(pp, input_file_benchmark_data)
            
            
            graph_combined_faces_error_over_floatparam(pp, input_file_benchmark_data, "regular", "param_step")
            graph_combined_faces_time_over_floatparam(pp, input_file_benchmark_data, "regular", "param_step")

            graph_combined_faces_error_over_floatparam(pp, input_file_benchmark_data, "curvature", "param_threshold")
            graph_combined_faces_time_over_floatparam(pp, input_file_benchmark_data, "curvature", "param_threshold")

            graph_combined_faces_error_over_floatparam(pp, input_file_benchmark_data, "terra", "param_max_error")
            graph_combined_faces_time_over_floatparam(pp, input_file_benchmark_data, "terra", "param_max_error")

            graph_combined_faces_error_over_floatparam(pp, input_file_benchmark_data, "zemlya", "param_max_error")        
            graph_combined_faces_time_over_floatparam(pp, input_file_benchmark_data, "zemlya", "param_max_error")

            graph_combined_num_faces_error_over_param_max_error_terra_vs_zemlya(pp, input_file_benchmark_data)
            graph_combined_num_faces_time_over_param_max_error_terra_vs_zemlya(pp, input_file_benchmark_data)
            graph_polygon_efficiency_over_param_max_error_terra_vs_zemlya(pp, input_file_benchmark_data)
        

    
def parse_commandline_arguments():
    parser = argparse.ArgumentParser("make graphs from tntn_benchmarks.csv")
    parser.add_argument("input_file", metavar="INPUT_FILE", type=str, help="the input .csv to read")
    parser.add_argument("output_file", metavar="OUTPUT_FILE", nargs="?", type=str, help="the output .pdf to produce")
  
    args = parser.parse_args()
    if args.output_file is None or len(args.output_file)==0 :
        args.output_file = '%s.pdf' % args.input_file
    
    return args
    
def main():
    args = parse_commandline_arguments()
    
    benchmarkscsv2pdf(args.input_file, args.output_file)
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
