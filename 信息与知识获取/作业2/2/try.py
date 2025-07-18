from process import get_file_paths, get_metadata
import os

def check_links():
    """检查所有文件的链接信息"""
    print("===== 链接诊断工具 =====")
    
    # 获取文件元数据
    metadata = get_metadata()
    
    print(f"共找到 {len(metadata)} 个文件")
    
    # 统计链接情况（遍历所有文件）
    links_found = 0
    empty_links = 0
    
    for item in metadata:
        url = item['url']
        if url and url.strip():
            links_found += 1
        else:
            empty_links += 1
    
    # 显示链接详情（只显示前20个）
    print("\n文件链接详情:")
    print("-" * 80)
    
    for i, item in enumerate(metadata):
        if i < 20:  # 只显示前20个文件的详情
            file_name = os.path.basename(item['file_path'])
            url = item['url']
            
            if url and url.strip():
                print(f"{i+1}. {file_name}: {url}")
            else:
                print(f"{i+1}. {file_name}: [无链接]")
    
    if len(metadata) > 20:
        print(f"... 省略 {len(metadata)-20} 个文件 ...")
    
    # 显示统计信息
    print("\n链接统计:")
    print(f"总文件数: {len(metadata)}")
    print(f"有链接的文件: {links_found} ({links_found/len(metadata)*100:.1f}%)")
    print(f"无链接的文件: {empty_links} ({empty_links/len(metadata)*100:.1f}%)")
    
    print("\n===== 诊断完成 =====")

if __name__ == "__main__":
    check_links()