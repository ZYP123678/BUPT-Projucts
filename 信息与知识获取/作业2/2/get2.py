import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin
import time
import os
import json
import re
import random

# 修改基础URL为BBC中文网
base_url = "https://www.bbc.com/zhongwen/simp"
headers = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"
}

def get_article_links(page_url):
    """获取页面上的文章链接"""
    response = requests.get(page_url, headers=headers)
    
    soup = BeautifulSoup(response.text, 'html.parser')
    
    # 收集所有可能的文章链接，匹配多种模式
    article_links = []
    
    # 模式1 - 标准文章链接
    links1 = soup.select('a[href*="/zhongwen/simp/"]')
    article_links.extend(links1)
    
    # 模式2 - 新格式文章链接 /zhongwen/articles/xxx/simp
    links2 = soup.select('a[href*="/zhongwen/articles/"][href*="/simp"]')
    article_links.extend(links2)
    
    # 模式3 - 日期模式文章链接
    links3 = soup.select('a[href*="-20"]')  # 大多数文章URL包含日期格式如"-20220101"
    article_links.extend(links3)
    
    # 如果在主页，也获取分类链接以便进一步抓取
    category_links = soup.select('a[href*="/zhongwen/topics/"][href*="/simp"]')
    
    print(f"找到 {len(links1)} 个标准格式链接")
    print(f"找到 {len(links2)} 个新格式链接")
    print(f"找到 {len(links3)} 个日期格式链接")
    print(f"找到 {len(category_links)} 个分类导航链接")
    
    links = []
    # 处理文章链接
    for a in article_links:
        href = a.get('href')
        if href:
            # 排除不需要的页面类型
            if any(x in href for x in ['/institutional/', '/help/', '/contact/', '/about/', '/termsofuse/']):
                continue
                
            # 确保链接是完整的URL
            if href.startswith('/'):
                full_url = urljoin("https://www.bbc.com", href)
                links.append(full_url)
            else:
                links.append(href)
    
    # 返回去重后的链接列表和分类链接
    category_urls = []
    for a in category_links:
        href = a.get('href')
        if href and href.startswith('/'):
            category_urls.append(urljoin("https://www.bbc.com", href))
        elif href:
            category_urls.append(href)
    
    # 去重前打印一些示例链接用于调试
    if links:
        print("示例链接:")
        for link in links[:5]:
            print(f"  - {link}")
    
    return list(set(links)), category_urls  # 使用set去除重复链接

def get_article_content(article_url):
    """获取文章的标题和内容"""
    response = requests.get(article_url, headers=headers)
    soup = BeautifulSoup(response.text, 'html.parser')
    
    # 获取文章标题 - 尝试多种可能的标题选择器
    title_elem = (
        soup.find('h1', class_=lambda c: c and 'bbc-' in c) or  # BBC中文网常用的标题类
        soup.find(['h1', 'h2']) or 
        soup.find('title')
    )
    title = title_elem.text.strip() if title_elem else "无标题"
    
    # 获取文章内容 - 尝试多种可能的内容选择器
    content_selectors = [
        'div[data-component="text-block"] p',  # 新版BBC文章段落
        'div.bbc-19j92fr p',  # 常见的BBC段落容器
        'article p',  # 通用文章段落
        '.story-body p',  # 旧版BBC文章
        '.bbc-ryelg3 p',  # 另一种常见的段落容器
        '.ssrcss-11r1m41-RichTextComponentWrapper p',  # 富文本包装器
        'p.bbc-1sy09mr',  # 新格式文章段落类
        '.body-content p'  # 另一种可能的内容类
    ]
    
    content_blocks = []
    for selector in content_selectors:
        blocks = soup.select(selector)
        if blocks:
            content_blocks = blocks
            print(f"使用选择器 '{selector}' 找到 {len(blocks)} 个段落")
            break
    
    # 如果上面的选择器都没找到内容，尝试获取所有段落
    if not content_blocks:
        content_blocks = soup.select('p')
        print(f"使用通用段落选择器，找到 {len(content_blocks)} 个段落")
    
    content = '\n\n'.join([p.text.strip() for p in content_blocks if p.text.strip()])
    
    # 检测是否为视频内容
    is_video = False
    if (
        '/av/' in article_url or 
        'av/' in article_url or
        soup.select('[data-testid="mediaplayer"]') or 
        soup.select('video') or
        "无法播放该视频" in content or
        "视频" in title or
        soup.find('div', class_=lambda c: c and ('media-player' in c or 'video' in c))
    ):
        is_video = True
        print("检测到视频内容")
    
    # 检测内容语言 - 确保是中文内容
    chinese_char_count = len(re.findall(r'[\u4e00-\u9fff]', content))
    if chinese_char_count < 10 and len(content) > 100:  # 如果中文字符太少，可能不是中文文章
        print("警告: 可能不是中文内容")
        is_chinese = False
    else:
        is_chinese = True
    
    # 打印标题和内容预览
    print(f"文章标题: {title}")
    if content:
        print(f"内容预览: {content[:100]}...")
    else:
        print("未找到内容")
    
    return {
        'title': title,
        'url': article_url,
        'content': content,
        'is_video': is_video,
        'is_chinese': is_chinese
    }

def save_article(article_data, output_dir='bbc_chinese_articles'):
    """保存文章到本地文件"""
    # 如果内容为空，则不保存
    if not article_data['content'].strip():
        print(f"跳过保存空内容: {article_data['title']}")
        return None
        
    # 只保存文章，不保存视频
    if article_data.get('is_video', False):
        article_dir = os.path.join(output_dir, 'videos')
    else:
        article_dir = os.path.join(output_dir, 'articles')
    
    # 创建保存目录
    if not os.path.exists(article_dir):
        os.makedirs(article_dir)
    
    # 使用标题作为文件名，处理不适合作为文件名的字符
    filename = re.sub(r'[\\/*?:"<>|]', "_", article_data['title'])
    filename = filename.strip()[:100]  # 限制文件名长度
    
    # 保存文章内容、标题和链接
    with open(os.path.join(article_dir, f"{filename}.txt"), 'w', encoding='utf-8') as f:
        f.write(f"标题: {article_data['title']}\n\n")
        f.write(f"链接: {article_data['url']}\n\n")
        f.write(f"内容类型: {'视频' if article_data.get('is_video', False) else '文章'}\n\n")
        f.write(f"内容:\n{article_data['content']}")
    
    return os.path.join(article_dir, f"{filename}.txt")

def save_article_index(articles, output_dir='bbc_chinese_articles'):
    """保存文章索引，包含所有文章的标题和链接"""
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # 分别创建文章和视频索引
    articles_only = [a for a in articles if not a.get('is_video', False)]
    videos_only = [a for a in articles if a.get('is_video', False)]
    
    # 保存JSON索引
    with open(os.path.join(output_dir, 'index.json'), 'w', encoding='utf-8') as f:
        json.dump(articles, f, ensure_ascii=False, indent=2)
    
    # 保存文章索引
    with open(os.path.join(output_dir, 'articles_index.txt'), 'w', encoding='utf-8') as f:
        f.write(f"BBC中文文章索引 (共{len(articles_only)}篇)\n\n")
        for i, article in enumerate(articles_only, 1):
            f.write(f"{i}. {article['title']}\n   {article['url']}\n\n")
    
    # 保存视频索引
    with open(os.path.join(output_dir, 'videos_index.txt'), 'w', encoding='utf-8') as f:
        f.write(f"BBC中文视频索引 (共{len(videos_only)}篇)\n\n")
        for i, article in enumerate(videos_only, 1):
            f.write(f"{i}. {article['title']}\n   {article['url']}\n\n")

def crawl_articles(max_articles=100):
    """爬取指定数量的文章链接"""
    all_links = []
    processed_links = set()  # 用于去重
    
    # 先从首页开始
    start_urls = [
        "https://www.bbc.com/zhongwen/simp"  # 首页
    ]
    
    # 首先获取首页及其分类链接
    category_urls = []
    for url in start_urls:
        print(f"正在爬取首页：{url}")
        links, categories = get_article_links(url)
        
        # 添加找到的链接
        for link in links:
            if link not in processed_links:
                all_links.append(link)
                processed_links.add(link)
        
        # 收集分类页面链接
        for cat in categories:
            if cat not in category_urls:
                category_urls.append(cat)
    
    print(f"找到 {len(category_urls)} 个分类页面")
    
    # 如果需要更多分类，可以手动添加
    additional_categories = [
        "https://www.bbc.com/zhongwen/simp/chinese-news",
        "https://www.bbc.com/zhongwen/simp/world",
        "https://www.bbc.com/zhongwen/simp/business",
        "https://www.bbc.com/zhongwen/simp/uk",
        "https://www.bbc.com/zhongwen/simp/indepth",
        "https://www.bbc.com/zhongwen/simp/science",
        # 添加新格式的页面
        "https://www.bbc.com/zhongwen/articles"
    ]
    
    for cat in additional_categories:
        if cat not in category_urls:
            category_urls.append(cat)
    
    # 遍历所有分类页面获取链接
    for cat_url in category_urls:
        if len(all_links) >= max_articles * 2:  # 获取两倍的链接数量，以应对可能的视频内容
            break
            
        print(f"正在爬取分类页面：{cat_url}")
        cat_links, _ = get_article_links(cat_url)
        
        # 添加新的未处理链接
        for link in cat_links:
            if link not in processed_links:
                all_links.append(link)
                processed_links.add(link)
                
        print(f"目前已收集 {len(all_links)} 个链接")
        time.sleep(random.uniform(1, 3))  # 随机延迟，避免被封
    
    print(f"共收集到 {len(all_links)} 个待处理链接")
    return all_links

def main():
    # 创建保存目录
    output_dir = 'bbc_chinese_articles'
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # 爬取文章链接
    print("开始爬取BBC中文网站文章...")
    article_links = crawl_articles(max_articles=200)  # 爬取更多链接，为了确保能得到100篇文章
    
    if not article_links:
        print("未能获取到任何文章链接，请检查网络连接或选择器是否正确")
        return
        
    print(f"获取到 {len(article_links)} 篇文章链接")
    
    # 获取并保存文章内容
    articles_data = []
    video_count = 0
    article_count = 0
    skipped_count = 0  # 记录跳过的空内容数量
    
    for i, link in enumerate(article_links, 1):
        try:
            print(f"\n正在处理第 {i}/{len(article_links)} 篇内容: {link}")
            article_data = get_article_content(link)
            
            # 跳过空内容
            if not article_data['content'].strip():
                print(f"跳过空内容页面: {article_data['title']}")
                skipped_count += 1
                continue
            
            # 跳过非中文内容
            if article_data.get('is_chinese') is False:
                print(f"跳过非中文内容: {article_data['title']}")
                skipped_count += 1
                continue
                
            # 只收集文章，跳过视频    
            if article_data.get('is_video', False):
                video_count += 1
                print(f"[视频内容] {article_data['title']} - 跳过")
                continue
            else:
                article_count += 1
                print(f"[文章内容] {article_data['title']}")
                
                file_path = save_article(article_data, output_dir)
                if file_path:  # 如果成功保存了文件
                    print(f"已保存到: {file_path}")
                    articles_data.append(article_data)
                    
                    # 达到100篇文章后退出
                    if article_count >= 100:
                        print(f"已达到目标数量: {article_count}篇文章")
                        break
            
            time.sleep(random.uniform(1, 2))  # 防止请求过快
        except Exception as e:
            print(f"处理内容时出错: {link}\n错误信息: {e}")
    
    # 保存索引
    save_article_index(articles_data, output_dir)
    print(f"文章索引已保存到 {output_dir}/articles_index.txt")
    print(f"成功保存了 {article_count} 篇文章到 {output_dir} 目录")
    print(f"跳过了 {skipped_count} 个空内容页面和 {video_count} 个视频内容")

if __name__ == "__main__":
    main()