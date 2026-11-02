#!/bin/bash

# 生成README_SUMMARY.md文件
echo '# SMP示例仓库' > README_SUMMARY.md

# 获取所有根目录下的文件夹（排除.git等隐藏文件夹）
root_dirs=$(find . -maxdepth 1 -type d -name "[!.]*" | sort | sed 's|^./||')

# 遍历每个根目录文件夹
for root_dir in $root_dirs; do
    # 跳过生成脚本所在的目录（如果需要）
    if [ "$root_dir" = "generate_readme_table.sh" ]; then
        continue
    fi
    
    # 添加二级标题
    echo -e "\n## $root_dir" >> README_SUMMARY.md
    
    # 获取当前根目录下的所有子文件夹
    sub_dirs=$(find "$root_dir" -maxdepth 1 -type d -name "[!.]*" | sort | sed 's|^./||')
    
    # 遍历每个子文件夹
    for sub_dir in $sub_dirs; do
        # 跳过当前根目录本身
        if [ "$sub_dir" = "$root_dir" ]; then
            continue
        fi
        
        # 检查是否有README.md或README.MD文件
        if [ -f "$sub_dir/README.md" ] || [ -f "$sub_dir/README.MD" ]; then
            # 确定README文件路径
            if [ -f "$sub_dir/README.md" ]; then
                readme_file="$sub_dir/README.md"
            else
                readme_file="$sub_dir/README.MD"
            fi
            
            # 提取README的一级标题
            title=$(grep -m 1 '^# ' "$readme_file" | sed 's/^# //')
            if [ -z "$title" ]; then
                title="无标题"
            fi
            
            # 添加一级列表项，带超链接
            echo "- [$title]($readme_file)" >> README_SUMMARY.md
        else
            # 检查是否有下一级子文件夹
            has_next_level=$(find "$sub_dir" -maxdepth 1 -type d -name "[!.]*" | grep -v "^$sub_dir$")
            
            if [ -n "$has_next_level" ]; then
                # 添加二级列表项（当前子文件夹名）
                echo "- $(basename "$sub_dir")" >> README_SUMMARY.md
                
                # 遍历下一级子文件夹
                next_level_dirs=$(find "$sub_dir" -maxdepth 1 -type d -name "[!.]*" | sort | sed 's|^./||')
                for next_level_dir in $next_level_dirs; do
                    # 跳过当前子文件夹本身
                    if [ "$next_level_dir" = "$sub_dir" ]; then
                        continue
                    fi
                    
                    # 检查是否有README.md或README.MD文件
                    if [ -f "$next_level_dir/README.md" ] || [ -f "$next_level_dir/README.MD" ]; then
                        # 确定README文件路径
                        if [ -f "$next_level_dir/README.md" ]; then
                            readme_file="$next_level_dir/README.md"
                        else
                            readme_file="$next_level_dir/README.MD"
                        fi
                        
                        # 提取README的一级标题
                        title=$(grep -m 1 '^# ' "$readme_file" | sed 's/^# //')
                        if [ -z "$title" ]; then
                            title="无标题"
                        fi
                        
                        # 添加三级列表项，带超链接
                        echo "  - [$title]($readme_file)" >> README_SUMMARY.md
                    else
                        # 检查是否还有更深层次的文件夹
                        has_deeper_level=$(find "$next_level_dir" -maxdepth 1 -type d -name "[!.]*" | grep -v "^$next_level_dir$")
                        
                        if [ -n "$has_deeper_level" ]; then
                            # 添加三级列表项（当前文件夹名）
                            echo "  - $(basename "$next_level_dir")" >> README_SUMMARY.md
                            
                            # 遍历更深层次的子文件夹
                            deeper_dirs=$(find "$next_level_dir" -maxdepth 1 -type d -name "[!.]*" | sort | sed 's|^./||')
                            for deeper_dir in $deeper_dirs; do
                                # 跳过当前文件夹本身
                                if [ "$deeper_dir" = "$next_level_dir" ]; then
                                    continue
                                fi
                                
                                # 检查是否有README.md或README.MD文件
                                if [ -f "$deeper_dir/README.md" ] || [ -f "$deeper_dir/README.MD" ]; then
                                    # 确定README文件路径
                                    if [ -f "$deeper_dir/README.md" ]; then
                                        readme_file="$deeper_dir/README.md"
                                    else
                                        readme_file="$deeper_dir/README.MD"
                                    fi
                                    
                                    # 提取README的一级标题
                                    title=$(grep -m 1 '^# ' "$readme_file" | sed 's/^# //')
                                    if [ -z "$title" ]; then
                                        title="无标题"
                                    fi
                                    
                                    # 添加四级列表项，带超链接
                                    echo "    - [$title]($readme_file)" >> README_SUMMARY.md
                                else
                                    # 标记为开发中
                                    echo "    - $(basename "$deeper_dir"): 开发中" >> README_SUMMARY.md
                                fi
                            done
                        else
                            # 标记为开发中
                            echo "  - $(basename "$next_level_dir"): 开发中" >> README_SUMMARY.md
                        fi
                    fi
                done
            else
                # 标记为开发中
                echo "- $(basename "$sub_dir"): 开发中" >> README_SUMMARY.md
            fi
        fi
    done
done

# 添加说明信息
echo -e "\n\n---\n\n**说明：**\n- 带有超链接的项目表示已有完整的README文档\n- 标记为"开发中"的项目表示该功能正在开发或尚未完成文档\n- 点击超链接可以查看详细的使用说明和示例代码" >> README_SUMMARY.md

# 提示完成
echo "README_SUMMARY.md 文件已生成完成！"